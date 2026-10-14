module UI.UIManager;

namespace ui
{
    // 世代の最大値(uint16_t)
    constexpr uint16_t MAX_GENERATION = 0xFFFF;

    void UIManager::Destroy(const UIElementID& id)
    {
        // 無効/古いIDは安全に無視
        if (!IsValid(id))
        {
            return;
        }

        m_elements.erase(id.index);
        ReleaseSlot(id.index);
    }

    void UIManager::Clear()
    {
        // 使用中の全スロットを解放し、Clear前のIDを無効化する
        m_elements.clear();
        for (uint32_t index = 0; index < m_slots.size(); ++index)
        {
            if (m_slots[index].alive)
            {
                ReleaseSlot(index);
            }
        }
    }

    void UIManager::ReleaseSlot(uint32_t index)
    {
        auto& slot = m_slots[index];
        slot.alive = false;
        if (slot.generation < MAX_GENERATION)
        {
            ++slot.generation;
            m_freeIndex.push(index);    // alive→falseの遷移時のみ登録されるため重複しない
        }
        // 世代が最大値のスロットは再利用せず退役させる(ラップアラウンドによる再有効化を防ぐ)
    }

    void UIManager::Update()
    {
        for (auto& element : m_elements)
        {
            element.second->Update();
        }
    }

    void UIManager::Draw()
    {
        for (auto& element : m_elements)
        {
            element.second->Draw();
        }
    }

    UIElementID UIManager::CreateID()
    {
        UIElementID id{};

        if (!m_freeIndex.empty())
        {
            // 再利用: 世代はReleaseSlotで進め済みなのでここでは進めない
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
}