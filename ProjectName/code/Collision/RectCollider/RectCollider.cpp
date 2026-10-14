module Collider.RectCollider;
import <memory>;
import MyLib.Math.Vector2;
import MyLib.Shape.Circle;
import Collider.CircleCollider;
import Collider.RectColliderVisitor;

using namespace math;

namespace col2d
{
    RectCollider::RectCollider(ColliderDef* def, const Vector2f& size)
        :Collider(def)
        , m_baseRect()
        , m_sweptRect()
    {
        // ビジターの初期化
        m_visitor = std::make_unique<RectColliderVisitor>(*this);

        // 矩形の初期化
        m_baseRect.pos = m_colDef->localPos;
        m_baseRect.size = size;

        // スイープ矩形の初期化
        m_sweptRect.pos = m_colDef->localPos;
        m_sweptRect.size = m_baseRect.size;
    }

    RectCollider::~RectCollider() = default;

    bool RectCollider::IsColliding(const Vector2f& point)
    {
        // スイープ矩形を使用して衝突判定
        if (m_colDef->shouldCCD)
        {
            return IsCollidingSegmentPoint(point);
        }

        //通常の衝突判定
        return m_baseRect.IsInside(point);
    }

    bool RectCollider::IsCollidingSegmentPoint(const Vector2f& point)
    {
        // スイープ矩形を使用して衝突判定
        CalcSweptRect(m_velocity);
        return m_sweptRect.IsInside(point);
    }

    bool RectCollider::IsColliding(const CircleCollider& circle)
    {
        // 連続衝突検出を行う場合
        if (m_colDef->shouldCCD || circle.GetColliderDef()->shouldCCD)
        {
            return IsCollidingSegmentCircle(circle);
        }


        // 中心から最も近い辺を求める
        Vector2f nearest
        {
            std::clamp(circle.GetCircle().center.x, m_baseRect.Left(), m_baseRect.Right()),
            std::clamp(circle.GetCircle().center.y, m_baseRect.Top(), m_baseRect.Bottom())
        };

        // 中心と辺の距離を計算して判定
        Vector2f diff = circle.GetCircle().center - nearest;
        return diff.LengthSq() <= (circle.GetCircle().radius * circle.GetCircle().radius);
    }

    bool RectCollider::IsColliding(const RectCollider& other)
    {
        // 連続衝突検出を行う場合
        if (m_colDef->shouldCCD || other.GetColliderDef()->shouldCCD)
        {
            return IsCollidingSegmentRect(other);
        }

        // 通常の矩形衝突判定
        return m_baseRect.AABB(other.GetRect());
    }

    bool RectCollider::IsCollidingSegmentRect(const RectCollider& other)
    {
        // 相対速度を計算
        Vector2f vRel = m_velocity - other.GetVelocity();
        if(vRel.LengthSq() < EPSILON)
        {
            // 相対速度がほぼゼロの場合、通常のAABB判定を行う
            return m_baseRect.AABB(other.GetRect());
        }

        // スイープ矩形を使用して衝突判定
        CalcSweptRect(vRel);
        return m_sweptRect.AABB(other.GetRect());

    }

    bool RectCollider::IsCollidingSegmentCircle(const CircleCollider& other)
    {
        // スイープ矩形を使用して衝突判定
        Vector2f vRel = m_velocity - other.GetVelocity();
        CalcSweptRect(vRel);

        // 中心から最も近い辺を求める
        Vector2f nearest
        {
            std::clamp(other.GetCircle().center.x, m_sweptRect.Left(), m_sweptRect.Right()),
            std::clamp(other.GetCircle().center.y, m_sweptRect.Top(), m_sweptRect.Bottom())
        };
        // 中心と辺の距離を計算して判定
        Vector2f diff = other.GetCircle().center - nearest;
        return diff.LengthSq() <= (other.GetCircle().radius * other.GetCircle().radius);
    }

    void RectCollider::CalcSweptRect(Vector2f velocity)
    {
        m_sweptRect.pos.x = std::min(m_colDef->localPos.x, m_colDef->localPos.x + velocity.x);
        m_sweptRect.pos.y = std::min(m_colDef->localPos.y, m_colDef->localPos.y + velocity.y);
        m_sweptRect.size = m_baseRect.size + Vector2f(std::abs(velocity.x), std::abs(velocity.y));
    }
}
