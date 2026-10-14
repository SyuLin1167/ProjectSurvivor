module Object.GameObject;
import GameSystem.FrameRate;

namespace object
{
    GameObject::GameObject()
        : m_compMgr(new component::ComponentManager)
        , m_deltaTime(gameSystem::FrameRate::GetDeltaTime())
        , m_pos()
        , m_velocity()
    {
        m_state = MainState::ACTIVE;
    }

    GameObject::~GameObject() = default;

    void GameObject::UpdateComponents()
    {
        m_compMgr->Update(m_deltaTime);
    }

    void GameObject::DrawComponents()
    {
        m_compMgr->Draw();
    }
}
