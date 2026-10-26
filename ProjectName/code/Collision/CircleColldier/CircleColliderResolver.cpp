module Collider.CircleColliderResolver;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;
import Collider.CircleCollider;
import Collider.RectCollider;

using namespace math;

namespace col2d
{
    std::optional<ContactInfo> CircleColliderResolver::Resolve(CircleCollider& issue, const RectCollider& other)
    {
        // 差分を算出
        shape::Rect otherRect = other.GetRect();
        Vector2f closestPoint
        {
            std::clamp(issue.GetCircle().center.x, otherRect.Left(), otherRect.Right()),
            std::clamp(issue.GetCircle().center.y, otherRect.Top(), otherRect.Bottom())
        };
        Vector2f diff = issue.GetCircle().center - closestPoint;
        
        // 判定距離の二乗を計算
        float distSq = diff.LengthSq();
        float radius = issue.GetCircle().radius;
        if (distSq >= radius * radius)
        {
            return std::nullopt;
        }

        // 押し戻し量と方向を計算
        float dist = std::sqrt(distSq);
        float penetration = radius - dist;
        Vector2f push;
        if (distSq > EPSILON * EPSILON)
            push = diff.Norm() * penetration;
        else
        {
            // 円中心が矩形内にあると差分が0になる。最も近い辺の外へ半径分まで押し出す。
            // 同値時は左・右・上・下の順で固定し、生成順に依存させない。
            const auto center = issue.GetCircle().center;
            float distance = center.x - otherRect.Left();
            Vector2f normal{-1, 0};
            if (otherRect.Right() - center.x < distance) { distance = otherRect.Right() - center.x; normal = {1, 0}; }
            if (center.y - otherRect.Top() < distance) { distance = center.y - otherRect.Top(); normal = {0, -1}; }
            if (otherRect.Bottom() - center.y < distance) { distance = otherRect.Bottom() - center.y; normal = {0, 1}; }
            push = normal * (distance + radius);
            closestPoint = center + normal * distance;
        }
        issue.AddVelocity(push);

        // 押し戻し後の速度成分除去
        Vector2f normal = push.Norm();
        Vector2f v = issue.GetVelocity();
        float vn = v.Dot(normal);
        issue.SetVelocity(v - normal * vn);

        // 衝突情報を作成
        ContactInfo info{};
        info.normal = normal;
        info.point = closestPoint;
        return info;
    }

    std::optional<ContactInfo> CircleColliderResolver::Resolve(CircleCollider& issue, const CircleCollider& other)
    {
        // 差分を算出
        Vector2f diff = issue.GetCircle().center - other.GetCircle().center;
        float distSq = diff.LengthSq();
        float rSum = issue.GetCircle().radius + other.GetCircle().radius;

        // 押し戻し量と方向を計算
        float dist = std::sqrt(distSq);
        float penetration = rSum - dist;
        Vector2f push = diff.Norm() * penetration;

        issue.AddVelocity(push);

        // 押し戻し後の速度成分除去
        Vector2f normal = push.Norm();
        Vector2f v = issue.GetVelocity();
        float vn = v.Dot(normal);
        issue.SetVelocity(v - normal * vn);

        // 衝突情報を作成
        ContactInfo info{};
        info.normal = normal;
        info.point = other.GetCircle().center + normal * other.GetCircle().radius;
        return info;
    }
}
