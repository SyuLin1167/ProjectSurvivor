module;
#include <DxLib.h>

module UI.Button;

import MyLib.Math.Vector2;
import MyLib.MouseStatus;
import <string>;

using namespace math;

namespace ui
{
    Button::Button(const ButtonDef* def)
        : m_def(std::make_unique<ButtonDef>(*def))
    {
        m_pos = m_def->buttonRect.pos;
        m_size = m_def->buttonRect.size;
    }

    Button::~Button() = default;

    void Button::Update()
    {
        // 不可視なら更新不要
        if (!IsVisible())
        {
            return;
        }

        // 座標とサイズを更新
        m_def->buttonRect.pos = m_pos;
        m_def->buttonRect.size = m_size;

        // マウス座標取得
        const Vector2i& mousePos = input::MouseStatus::GetMousePos();

        // ボタンのイベントを実施
        if (m_def->buttonRect.IsInside({ mousePos.x, mousePos.y }))
        {
            if (m_def->onHover)
            {
                m_def->onHover();
            }
            OnClick();
            OnReleased();
        }
    }

    void Button::Draw()
    {
        // 不可視なら描画不要
        if (!IsVisible())
        {
            return;
        }

        // ボタン描画
        DrawBoxAA(m_def->buttonRect.pos.x, m_def->buttonRect.pos.y,
            m_def->buttonRect.pos.x + m_def->buttonRect.size.x, m_def->buttonRect.pos.y + m_def->buttonRect.size.y,
            m_def->color, true);

        // テキスト描画
        DrawString(static_cast<int>(m_def->buttonRect.pos.x + 8),
            static_cast<int>(m_def->buttonRect.pos.y + 8),
            m_def->text.c_str(), GetColor(0, 0, 0));
    }

    void Button::OnClick()
    {
        // クリック状態チェック
        if (input::MouseStatus::CheckClick(MOUSE_INPUT_LEFT, CLICK_ON_PRESS))
        {
            // クリック時イベント実行
            if (m_def->onClicked)
            {
                m_def->onClicked();
            }
            m_onClicked = true;
        }
    }

    void Button::OnReleased()
    {
        // クリック解除状態チェック
        if (m_onClicked)
        {
            if (input::MouseStatus::CheckClick(MOUSE_INPUT_LEFT, CLICK_ON_RELEASE))
            {
                // クリック解除時イベント実行
                if (m_def->onReleased)
                {
                    m_def->onReleased();
                }
                m_onClicked = false;
            }
        }
    }

}