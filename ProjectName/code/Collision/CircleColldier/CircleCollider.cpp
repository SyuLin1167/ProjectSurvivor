module Collider.CircleCollider;

import MyLib.Shape.Circle;
import MyLib.Shape.Rect;
import MyLib.Math.Vector2;
import Collider.CircleColliderVisitor;
import Collider.RectCollider;

using namespace math;

namespace col2d
{
    CircleCollider::CircleCollider(ColliderDef* def, float radius)
        : Collider(def)
        , m_baseCircle()
    {
        // ビジターの初期化
        m_visitor = std::make_unique<CircleColliderVisitor>(*this);

        // 円の初期化
        m_baseCircle.center = def->localPos;
        m_baseCircle.radius = radius;
    }

    CircleCollider::~CircleCollider() = default;

    bool CircleCollider::IsCollider(const Vector2f& point)
    {
        // 連続衝突検出を行う場合
        if (m_colDef->shouldCCD)
        {
            return IsColliderSegmentPoint(point);
        }

        // 通常判定
        return m_baseCircle.IsInside(point);
    }

    inline bool CircleCollider::IsColliderSegmentPoint(const Vector2f& point)
    {
        // 移動ベクトルの長さの二乗を計算
        float lenSq = m_velocity.LengthSq();
        float t = 0.0f;

        // 移動ベクトルに対する射影を計算し、0から1の範囲にクランプ
        if (lenSq > EPSILON)
        {
            t = (point - m_baseCircle.center).Dot(m_velocity) / lenSq;
            t = std::clamp(t, 0.0f, 1.0f);
        }

        // 最も近い点を計算して判定
        Vector2f closest = m_baseCircle.center + m_velocity * t;
        Vector2f diff = point - closest;
        return diff.LengthSq() <= m_baseCircle.radius * m_baseCircle.radius;
    }

    bool CircleCollider::IsCollider(const RectCollider& other)
    {
        // 連続衝突検出を行う場合
        if(m_colDef->shouldCCD || other.GetColliderDef()->shouldCCD)
        {
            return IsColliderSegmentRect(other.GetRect());
        }

        // 中心から最も近い辺を求める
        Vector2f nearest
        {
            std::clamp(m_baseCircle.center.x, other.GetRect().Left(), other.GetRect().Right()),
            std::clamp(m_baseCircle.center.y, other.GetRect().Top(), other.GetRect().Bottom())
        };

        // 中心と辺の距離を計算して判定
        Vector2f diff = m_baseCircle.center - nearest;
        return diff.LengthSq() <= (m_baseCircle.radius * m_baseCircle.radius);
    }

    inline bool CircleCollider::IsColliderSegmentRect(const shape::Rect& rect)
    {
        // 拡張された矩形の境界を計算
        float minX = rect.Left() - m_baseCircle.radius;
        float maxX = rect.Right() + m_baseCircle.radius;
        float minY = rect.Top() - m_baseCircle.radius;
        float maxY = rect.Bottom() + m_baseCircle.radius;

        // 時間の初期化
        float tMin = 0.0f;
        float tMax = 1.0f;

        // X軸の判定
        if (std::fabs(m_velocity.x) < EPSILON)
        {
            // X軸に沿って移動しない場合
            if (m_baseCircle.center.x < minX || m_baseCircle.center.x > maxX)
            {
                return false;
            }
        }
        else
        {
            // X軸の移動に対する逆数を計算
            float invDx = 1.0f / m_velocity.x;
            float t1 = (minX - m_baseCircle.center.x) * invDx;
            float t2 = (maxX - m_baseCircle.center.x) * invDx;
            if (t1 > t2)
            {
                std::swap(t1, t2);
            }
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            // 交差しない場合
            if (tMax < tMin)
            {
                return false;
            }
        }

        // Y軸の判定
        if (std::fabs(m_velocity.y) < EPSILON)
        {
            // Y軸に沿って移動しない場合
            if (m_baseCircle.center.y < minY || m_baseCircle.center.y > maxY)
            {
                return false;
            }
        }
        else
        {
            // Y軸の移動に対する逆数を計算
            float invDy = 1.0f / m_velocity.y;
            float t1 = (minY - m_baseCircle.center.y) * invDy;
            float t2 = (maxY - m_baseCircle.center.y) * invDy;
            if (t1 > t2)
            {
                std::swap(t1, t2);
            }
            tMin = std::max(tMin, t1);
            tMax = std::min(tMax, t2);

            // 交差しない場合
            if (tMax < tMin)
            {
                return false;
            }
        }

        // 0～1 区間に交差時刻が存在するかどうか
        return (tMin <= 1.0f && tMax >= 0.0f);
    }

    bool CircleCollider::IsCollider(const CircleCollider& other)
    {
        // 連続衝突検出を行う場合
        if(m_colDef->shouldCCD || other.GetColliderDef()->shouldCCD)
        {
            return IsColliderSegmentCircle(other);
        }

        return m_baseCircle.IsInside(other.m_baseCircle);
    }

    inline bool CircleCollider::IsColliderSegmentCircle(const CircleCollider& other)
    {
        // 相対速度の算出
        Vector2f vRel = m_velocity - other.GetVelocity();
        Vector2f d = m_baseCircle.center - other.m_baseCircle.center;
        float r = m_baseCircle.radius + other.m_baseCircle.radius;

        // 初期重なり判定
        if (d.LengthSq() <= r * r)
        {
            return true;
        }

        // 相対速度が0の場合は衝突しない
        if (vRel.LengthSq() < EPSILON)
        {
            return false;
        }

        // 二次方程式の係数を計算
        float b = 2.0f * d.Dot(vRel);
        float c = d.LengthSq() - r * r;

        // 判別式を計算して衝突判定
        float discriminant = b * b - 4.0f * vRel.LengthSq() * c;
        if (discriminant < 0.0f)
        {
            return false;
        }

        // 判別式が0の場合は接触している
        float sqrtDisc = std::sqrt(discriminant);
        float t = (-b - sqrtDisc) / (2.0f * vRel.LengthSq());

        return (t >= 0.0f && t <= 1.0f);
    }

    void CircleCollider::Accept(ColliderVisitor& visitor)
    {
        visitor.Visit(*this);
    }
}
