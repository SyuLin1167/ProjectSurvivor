module;
#include <math.h>
#include <limits>

module Collider.TileColliderResolver;
import <optional>;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import Collider.TileCollider;

using namespace math;

namespace col2d
{
    constexpr float INF = std::numeric_limits<float>::max();    // 無限大
    static constexpr float ONE_WAY_EPS = 5.0f;                  // 足場判定の許容誤差


    std::optional<ContactInfo> TileColliderResolver::Resolve(RectCollider& issue, const TileInfo& tileInfo)
    {
        switch (tileInfo.type)
        {
        case TileType::SOLID:
            return ResolveSolid(issue, tileInfo);
        case TileType::ONE_WAY_TOP:
            return ResolveOneWayTop(issue, tileInfo);
        default:
            return std::nullopt;
        }
    }

    Vector2f TileColliderResolver::CalcPushBack(const RectCollider& issue, const uint8_t adjacentFlag, const shape::Rect& tileRect)
    {
        // めり込み量を計算(隣接タイルがある方向は無効化)
        Vector2f diffX{};
        diffX.x = (adjacentFlag & TileFlag::LEFT) ? INF : tileRect.Left() - issue.GetRect().Right();
        diffX.y = (adjacentFlag & TileFlag::RIGHT) ? INF : tileRect.Right() - issue.GetRect().Left();

        Vector2f diffY{};
        diffY.x = (adjacentFlag & TileFlag::TOP) ? INF : tileRect.Top() - issue.GetRect().Bottom();
        diffY.y = (adjacentFlag & TileFlag::BOTTOM) ? INF : tileRect.Bottom() - issue.GetRect().Top();

        // 軸ごと押し戻し量を出す
        float dx = (std::abs(diffX.x) < std::abs(diffX.y)) ? diffX.x : diffX.y;
        float dy = (std::abs(diffY.x) < std::abs(diffY.y)) ? diffY.x : diffY.y;

        return Vector2f(dx, dy);
    }

    std::optional<ContactInfo> TileColliderResolver::ResolveSolid(RectCollider& issue, const TileInfo& tileInfo)
    {

        // 押し戻し量計算用定数
        const uint8_t adjacentFlag = tileInfo.adjacentFlag;
        const shape::Rect& tileRect = tileInfo.collider->GetRect();

        // 押し戻し量計算
        Vector2f pushBack = CalcPushBack(issue, adjacentFlag, tileRect);

        ContactInfo info{};

        if (abs(pushBack.x) < abs(pushBack.y))
        {
            // 押し戻しが大きすぎる場合は無視
            if (fabs(pushBack.x) >= tileRect.size.x)
            {
                return std::nullopt;
            }

            // 移動方向と同じ方向に押し戻さない
            if ((pushBack.x > 0.0f && issue.GetVelocity().x > 0.0f) ||
                (pushBack.x < 0.0f && issue.GetVelocity().x < 0.0f))
            {
                return std::nullopt;
            }

            // 押し戻し量を反映
            issue.AddVelocity(Vector2f(pushBack.x, 0.0f));
            issue.SetVelocity(Vector2f(0.0f, issue.GetVelocity().y));

            // 衝突情報を作成
            info.normal = std::signbit(pushBack.x) ? NORMAL_LEFT : NORMAL_RIGHT;
            info.point = tileRect.Center();
            info.point.x = std::signbit(pushBack.x) ? tileRect.Left() : tileRect.Right();
        }
        else
        {
            // 押し戻しが大きすぎる場合は無視
            if (fabs(pushBack.y) >= tileRect.size.y)
            {
                return std::nullopt;
            }

            // 移動方向と同じ方向に押し戻さない
            if ((pushBack.y > 0.0f && issue.GetVelocity().y > 0.0f) ||
                (pushBack.y < 0.0f && issue.GetVelocity().y < 0.0f))
            {
                return std::nullopt;
            }

            // 押し戻し量を反映
            issue.AddVelocity(Vector2f(0.0f, pushBack.y));
            issue.SetVelocity(Vector2f(issue.GetVelocity().x, 0.0f));

            // 衝突情報を作成
            info.normal = std::signbit(pushBack.y) ? NORMAL_TOP : NORMAL_BOTTOM;
            info.point = tileRect.Center();
            info.point.y = std::signbit(pushBack.y) ? tileRect.Top() : tileRect.Bottom();
        }

        return info;
    }

    std::optional<ContactInfo> TileColliderResolver::ResolveOneWayTop(RectCollider& issue, const TileInfo& tileInfo)
    {
        // 下方向（落下中）のみ判定
        if (issue.GetVelocity().y <= 0.0f)
        {
            return std::nullopt;
        }

        const shape::Rect& tileRect = tileInfo.collider->GetRect();
        const shape::Rect& issueRect = issue.GetRect();

        // 水平オーバーラップなしなら無視
        if (issueRect.Right() <= tileRect.Left() || issueRect.Left() >= tileRect.Right())
        {
            return std::nullopt;
        }

        // 侵入していなければ無視
        if (issueRect.Bottom() <= tileRect.Top())
        {
            return std::nullopt;
        }

        // 前フレームで既に足場内部 / 下側から侵入していた場合は無視
        float prevBottom = issueRect.Bottom() - issue.GetVelocity().y;
        if (prevBottom > tileRect.Top() + ONE_WAY_EPS)
        {
            return std::nullopt;
        }

        // 押し戻し量
        float penetration = tileRect.Top() - issueRect.Bottom();

        // 想定外に大きすぎる侵入は無視
        if (fabs(penetration) >= tileRect.size.y)
        {
            return std::nullopt;
        }

        issue.AddVelocity(Vector2f(0.0f, penetration));
        issue.SetVelocity(Vector2f(issue.GetVelocity().x, 0.0f));

        ContactInfo info{};
        info.normal = NORMAL_TOP;
        info.point = tileRect.Center();
        info.point.y = tileRect.Top();
        return info;
    }
};
