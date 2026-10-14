module Component.ComponentManager;

namespace component
{
    void ComponentManager::Update(const float& deltaTime)
    {
        for (auto& component : m_components)
        {
            component.second->Update(deltaTime);
        }
    }

    void ComponentManager::Draw()
    {
        for (auto& component : m_components)
        {
            component.second->Draw();
        }
    }
}
