module;
#include "MovementMath.h"

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
        const auto held = [](int primary, int alternative)
        {
            return KeyStatus::CheckKey(primary, ON_PRESS | PRESSING) ||
                (alternative >= 0 && KeyStatus::CheckKey(alternative, ON_PRESS | PRESSING));
        };
        const auto direction = movement::ResolveDirection(
            m_canMoveX && held(m_leftKey, m_altLeft),
            m_canMoveX && held(m_rightKey, m_altRight),
            m_canMoveY && held(m_upKey, m_altUp),
            m_canMoveY && held(m_downKey, m_altDown), m_normalizeDiagonal);
        if (m_canMoveX)
            m_owner->AccessVel().Assign(PropVector2<float>::X, direction.x * m_moveSpeed.x * deltaTime);
        if (m_canMoveY)
            m_owner->AccessVel().Assign(PropVector2<float>::Y, direction.y * m_moveSpeed.y * deltaTime);
    }
}
