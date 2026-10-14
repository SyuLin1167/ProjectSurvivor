module Collider.CircleColliderVisitor;

import MyLib.Math.Vector2;
import Collider.CircleCollider;
import Collider.RectCollider;

using namespace math;

namespace col2d
{
    CircleColliderVisitor::CircleColliderVisitor(CircleCollider& issue)
        : m_issue(issue)
    {
        // 処理なし
    }

    void CircleColliderVisitor::Visit(RectCollider& target)
    {
        // 無効なら処理しない
        if (!m_issue.GetColliderDef()->isActive || !target.GetColliderDef()->isActive)
        {
            return;
        }

        m_hadContact = false;

        // サブステップ計算
        Vector2f vel = m_issue.GetVelocity();
        float longest = std::max(std::abs(vel.x), std::abs(vel.y));
        float stepMax = std::max(0.0001f, m_issue.GetCircle().radius * 0.5f);
        int N = std::max(1, static_cast<int>(std::ceil(longest / stepMax)));
        vel /= static_cast<float>(N);
        Vector2f accumulatedMove{ 0.0f, 0.0f };

        for (int i = 0; i < N; ++i)
        {
            // 位置更新
            m_issue.AddVelocity(vel);
            accumulatedMove += vel;

            // 衝突検出
            if (m_issue.IsCollider(target))
            {
                // 衝突処理を行い、続行不可ならループを抜ける
                if (!HandleContact<CircleCollider, RectCollider, CircleColliderResolver>(m_issue, target, m_resolver))
                {
                    break;
                }
            }
        }

        // 衝突していればイベントを実施する
        if (m_hadContact)
        {
            m_issue.TriggerEvent(target.GetFilter().category, m_info);
            m_info.normal *= -1.0f; // 法線を反転させる
            target.TriggerEvent(m_issue.GetFilter().category, m_info);
        }

        // サブステップで進めた純粋な前進分だけを必ず巻き戻す
        m_issue.AddVelocity(accumulatedMove * -1.0f);
    }

    void CircleColliderVisitor::Visit(CircleCollider& target)
    {
        // 無効なら処理しない
        if (!m_issue.GetColliderDef()->isActive || !target.GetColliderDef()->isActive)
        {
            return;
        }
        m_hadContact = false;
        
        // サブステップ計算
        Vector2f vel = m_issue.GetVelocity();
        float longest = std::max(std::abs(vel.x), std::abs(vel.y));
        float stepMax = std::max(0.0001f, m_issue.GetCircle().radius * 0.5f);
        int N = std::max(1, static_cast<int>(std::ceil(longest / stepMax)));
        vel /= static_cast<float>(N);
        Vector2f accumulatedMove{ 0.0f, 0.0f };

        for (int i = 0; i < N; ++i)
        {
            // 位置更新
            m_issue.AddVelocity(vel);
            accumulatedMove += vel;

            // 衝突検出
            if (m_issue.IsCollider(target))
            {
                // 衝突処理を行い、続行不可ならループを抜ける
                if (!HandleContact<CircleCollider, CircleCollider, CircleColliderResolver>(m_issue, target, m_resolver))
                {
                    break;
                }
            }
        }

        // 衝突していればイベントを実施する
        if (m_hadContact)
        {
            m_issue.TriggerEvent(target.GetFilter().category, m_info);
            m_info.normal *= -1.0f; // 法線を反転させる
            target.TriggerEvent(m_issue.GetFilter().category, m_info);
        }

        // サブステップで進めた純粋な前進分だけを必ず巻き戻す
        m_issue.AddVelocity(accumulatedMove * -1.0f);
    }
}