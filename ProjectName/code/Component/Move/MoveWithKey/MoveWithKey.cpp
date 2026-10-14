module Component.MoveWithKey;

import MyLib.KeyStatus;
import Object.GameObject;

using namespace math;
using namespace input;

namespace component
{
    MoveWithKey::MoveWithKey(object::GameObject* owner)
        :ComponentBase(owner)
        , m_leftKey(0)
        , m_rightKey(0)
        , m_upKey(0)
        , m_downKey(0)
        , m_canMoveX(false)
        , m_canMoveY(false)
    {
        // 処理なし
    }

    void MoveWithKey::SetHorizontal(const int leftKey, const int rightKey, float moveSpeed)
    {
        m_canMoveX = true;
        m_leftKey = leftKey;
        m_rightKey = rightKey;
        m_moveSpeed.x = moveSpeed;
    }

    void MoveWithKey::SetVertical(const int upKey, const int downKey, float moveSpeed)
    {
        m_canMoveY = true;
        m_upKey = upKey;
        m_downKey = downKey;
        m_moveSpeed.y = moveSpeed;
    }

    void MoveWithKey::Update(const float& deltaTime)
    {
        Vector2f direction{};

        //移動処理
        if (m_canMoveX)
        {
            if (KeyStatus::CheckKey(m_leftKey, ON_PRESS | PRESSING))
            {
                direction -= 1.0f;
            }
            if (KeyStatus::CheckKey(m_rightKey, ON_PRESS | PRESSING))
            {
                direction += 1.0f;
            }
            m_owner->AccessVel().Assign(PropVector2<float>::X, direction.x * m_moveSpeed.x * deltaTime);
        }
        if (m_canMoveY)
        {
            if (KeyStatus::CheckKey(m_upKey, ON_PRESS | PRESSING))
            {
                direction.y -= 1.0f;
            }

            if (KeyStatus::CheckKey(m_downKey, ON_PRESS | PRESSING))
            {
                direction.y += 1.0f;
            }
            m_owner->AccessVel().Assign(PropVector2<float>::Y, direction.y * m_moveSpeed.x * deltaTime);
        }
    }
}
