module Collider.TileColliderVisitor;
import MyLib.Math.Vector2;
import Collider.TileCollider;
import Collider.RectCollider;

using namespace math;

namespace col2d
{
    TileColliderVisitor::TileColliderVisitor(TileCollider& issue)
        : m_issue(issue)
    {
        // 処理なし
    }

    void TileColliderVisitor::Visit(RectCollider& collider)
    {
        // コライダーが有効でない場合は処理を行わない
        if (!m_issue.GetColliderDef()->isActive || !collider.GetColliderDef()->isActive)
        {
            return;
        }

        // 元の移動量を保存
        const Vector2f velocity = collider.GetVelocity();

        // 解決処理側で参照できるように移動量を保持
        collider.SetVelocity(velocity);

        // X方向
        collider.AddVelocity({ velocity.x, 0.0f });
        if (m_issue.IsColliding(collider.GetRect()))
        {
            ProcessCollisionTiles(collider);
        }

        // Y方向
        collider.AddVelocity({ 0.0f, velocity.y });
        if (m_issue.IsColliding(collider.GetRect()))
        {
            ProcessCollisionTiles(collider);
        }
    }

    void TileColliderVisitor::ProcessCollisionTiles(RectCollider& collider)
    {
        auto hitTileKeys = m_issue.TakeHitTileKeys();

        while (!hitTileKeys.empty())
        {
            // 衝突タイル情報取得
            auto [key, index] = hitTileKeys.front();
            hitTileKeys.pop();
            auto tileInfo = m_issue.GetTileInfo(key, index);

            // タイルコライダーが存在しない場合はスキップ
            if (!tileInfo || !tileInfo->collider)
            {
                continue;
            }

            // 現在位置で本当に衝突しているか再確認
            if (collider.IsColliding(*tileInfo->collider))
            {
                const auto contact = m_resolver.Resolve(collider, *tileInfo);

                // 衝突箇所があれば衝突として確定しイベント発火
                if (contact)
                {
                    collider.TriggerEvent(m_issue.GetFilter().category, *contact);
                }
            }
        }
    }
}