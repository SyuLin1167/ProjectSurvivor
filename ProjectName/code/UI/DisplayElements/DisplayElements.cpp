module;
#include <DxLib.h>
#include <algorithm>
module UI.DisplayElements;
namespace ui
{
    void TextLabel::Draw()
    {
        if (!IsVisible()) return;
        DrawString(static_cast<int>(m_pos.x), static_cast<int>(m_pos.y), m_text.c_str(), m_color);
    }
    void Gauge::Draw()
    {
        if (!IsVisible()) return;
        const int x = static_cast<int>(m_pos.x), y = static_cast<int>(m_pos.y);
        const int width = static_cast<int>((std::max)(0.0f, m_size.x));
        const int height = static_cast<int>((std::max)(0.0f, m_size.y));
        DrawBox(x, y, x + width, y + height, m_background, true);
        const int fill = static_cast<int>(width * m_ratio);
        if (fill > 0) DrawBox(x, y, x + fill, y + height, m_fill, true);
        DrawBox(x, y, x + width, y + height, m_border, false);
    }
}
