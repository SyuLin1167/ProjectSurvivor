module Component.Jump;
import Object.GameObject;
import MyLib.Math.PropVector2;

using namespace math;

namespace component
{
    Jump::Jump(object::GameObject* owner, const float& jumpPower, std::function<bool()> trigger)
        :ComponentBase(owner)
        , JUMP_POWER(jumpPower)
        , m_isGround(false)
        , m_verticalSpeed(0.0f)
        , m_trigger(trigger)
    {
        //処理なし
    }

    void Jump::Update(const float& deltaTime)
    {
        //トリガーが引かれたらジャンプを実施
        if (m_trigger())
        {
            TryJump(deltaTime);
        }

        // 空中なら最大落下速度まで重力を加算する
        if (!m_isGround)
        {
            m_verticalSpeed += GRAVITY;
            if(m_verticalSpeed> MAX_FALL_VELOCITY)
            {
                m_verticalSpeed = MAX_FALL_VELOCITY;
            }
        }

        // Y軸の速度をオーナーに反映
        m_owner->AccessVel().Assign(PropVector2<float>::Y, m_verticalSpeed * deltaTime);

        m_isGround = false; //地面に接地しているかどうかのフラグをリセット
    }

    void Jump::TryJump(const float& deltaTime)
    {
        //ジャンプを実施
        if (m_canJump)
        {
            m_canJump = false;
            m_isGround = false;
            m_verticalSpeed = -JUMP_POWER;
        }
    }
}
