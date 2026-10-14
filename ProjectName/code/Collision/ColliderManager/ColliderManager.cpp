module Collider.ColliderManager;

namespace col2d
{
    // 世代の最大値(uint16_t)
    constexpr uint16_t MAX_GENERATION = 0xFFFF;

    ColliderID ColliderManager::CreateID()
    {
        ColliderID id{};

        if (!m_freeIndex.empty())
        {
            // 再利用: 世代はDestroy時に進め済みなのでここでは進めない
            id.index = m_freeIndex.front();
            m_freeIndex.pop();
        }
        else
        {
            // 新規: スロットを追加(サイズは減らないため単調増加)
            id.index = static_cast<uint32_t>(m_slots.size());
            m_slots.emplace_back();
        }

        m_slots[id.index].alive = true;
        id.generation = m_slots[id.index].generation;
        return id;
    }

    void ColliderManager::Destroy(const ColliderID& id)
    {
        // 無効/古いIDは安全に無視
        if (!IsValid(id))
        {
            return;
        }

        if (auto it = m_colliders.find(id.index); it != m_colliders.end())
        {
            // 他のコライダーからのマスクとイベントを削除
            auto& category = it->second->GetFilter().category;
            for (auto& [index, collider] : m_colliders)
            {
                collider->GetFilter().RemoveMask(category);
                collider->DeleteEvent(category);
            }

            m_colliders.erase(it);
        }

        // スロットを解放し世代を進める(古いIDはこれ以降無効)
        auto& slot = m_slots[id.index];
        slot.alive = false;
        if (slot.generation < MAX_GENERATION)
        {
            ++slot.generation;
            m_freeIndex.push(id.index);     // alive→falseの遷移時のみ登録されるため重複しない
        }
        // 世代が最大値のスロットは再利用せず退役(ラップアラウンドで古いIDが再有効化されるのを防ぐ)
    }

    Collider* ColliderManager::GetCollider(const ColliderID& id) const
    {
        if (!IsValid(id))
        {
            return nullptr;
        }

        if (auto it = m_colliders.find(id.index); it != m_colliders.end())
        {
            return it->second.get();
        }

        return nullptr;
    }

    void ColliderManager::Step()
    {
        // コライダーの更新処理
        for (auto& [index, collider] : m_colliders)
        {
            for (auto& otherCollider : m_colliders)
            {
                // 同じコライダー同士の衝突は無視
                if (collider == otherCollider.second)
                {
                    continue;
                }

                // フィルターを確認して衝突判定を行う
                if (collider->GetFilter().HasMask(otherCollider.second->GetFilter().category))
                {
                    // コライダー同士の衝突判定
                    collider->CollideWith(*otherCollider.second);
                }
            }

            collider->AddVelocity();
        }
    }

    }
