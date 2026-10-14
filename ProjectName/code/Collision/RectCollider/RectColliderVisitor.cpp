module Collider.RectColliderVisitor;

import MyLib.Math.Vector2;
import Collider.RectCollider;
import Collider.CircleCollider;

using namespace math;

namespace col2d
{
    RectColliderVisitor::RectColliderVisitor(RectCollider& issue)
        :m_issue(issue)
    {
        // 処理なし
    }

    void RectColliderVisitor::Visit(col2d::RectCollider& target)
    {
        // コライダーが有効でない場合は処理を行わない
        if (!m_issue.GetColliderDef()->isActive || !target.GetColliderDef()->isActive)
        {
            return;
        }
        m_hadContact = false;

        // サブステップ数を決定
        Vector2f vel = m_issue.GetVelocity();
        float longest = std::max(std::abs(vel.x), std::abs(vel.y));
        auto selfRect = m_issue.GetRect();
        float stepMax = std::max(0.0001f, std::min(selfRect.size.Half().x, selfRect.size.Half().y));
        int N = std::max(1, static_cast<int>(std::ceil(longest / stepMax)));
        vel /= static_cast<float>(N);
        Vector2f accumulatedMove{ 0.0f, 0.0f };

        // サブステップで仮想移動して衝突解決
        for (int i = 0; i < N; ++i)
        {
            // 仮想移動
            m_issue.AddVelocity(vel);
            accumulatedMove += vel;

            // 衝突していたら解決する
            if (m_issue.IsColliding(target))
            {
                // 衝突処理を行い、続行不可ならループを抜ける
                if (!HandleContact<RectCollider, RectCollider, RectColliderResolver>(m_issue, target, m_resolver))
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

    void RectColliderVisitor::Visit(col2d::CircleCollider& target)
    {
        // コライダーが有効でない場合は処理を行わない
        if (!m_issue.GetColliderDef()->isActive || !target.GetColliderDef()->isActive)
        {
            return;
        }
        m_hadContact = false;

        // サブステップ数を決定
        Vector2f vel = m_issue.GetVelocity();
        float longest = std::max(std::abs(vel.x), std::abs(vel.y));
        auto selfRect = m_issue.GetRect();
        float stepMax = std::max(0.0001f, std::min(selfRect.size.Half().x, selfRect.size.Half().y));
        int N = std::max(1, static_cast<int>(std::ceil(longest / stepMax)));
        vel /= static_cast<float>(N);
        Vector2f accumulatedMove{ 0.0f, 0.0f };

        // サブステップで仮想移動して衝突解決
        for (int i = 0; i < N; ++i)
        {
            // 仮想移動
            m_issue.AddVelocity(vel);
            accumulatedMove += vel;

            // 衝突していたら解決する
            if (m_issue.IsColliding(target))
            {
                // 衝突処理を行い、続行不可ならループを抜ける
                if (!HandleContact<RectCollider, CircleCollider, RectColliderResolver>(m_issue, target, m_resolver))
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
};
