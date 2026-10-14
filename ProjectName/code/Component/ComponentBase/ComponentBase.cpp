module Component;
import Object.GameObject;

namespace component
{
    ComponentBase::ComponentBase(object::GameObject* owner)
        : m_owner(owner)
    {
        //処理なし
    }
}