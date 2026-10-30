module;
#include <algorithm>
#include <string>
export module UI.DisplayElements;
import UI.UIBase;

export namespace ui
{
    /// <summary>位置・色・文字列を設定できる、画面座標の文字UI。</summary>
    class TextLabel : public UIBase
    {
    public:
        /// <summary>文字と描画色を保持する。</summary>
        /// <param name="text">表示文字列。書式指定ではなく、そのまま描く。</param>
        /// <param name="color">DxLibの描画色。</param>
        TextLabel(const std::string& text = "", unsigned int color = 0xffffff) : m_text(text), m_color(color) {}
        /// <summary>表示文字列を差し替える。</summary>
        /// <param name="text">新しい文字列。</param>
        void SetText(const std::string& text) { m_text = text; }
        /// <summary>描画色を変更する。</summary>
        /// <param name="color">DxLibの描画色。</param>
        void SetColor(unsigned int color) { m_color = color; }
        /// <summary>現在の文字列を取得する。</summary>
        /// <returns>この要素が保持する文字列。</returns>
        const std::string& Text() const { return m_text; }
        /// <summary>表示中の場合だけ文字を描く。</summary>
        void Draw() override;
    private:
        std::string m_text;
        unsigned int m_color;
    };

    /// <summary>ゲームのHPなどに依存しない横ゲージ。位置・サイズはUIBaseから設定する。</summary>
    class Gauge : public UIBase
    {
    public:
        /// <summary>背景・塗り・枠の描画色を設定する。</summary>
        /// <param name="background">背景色。</param>
        /// <param name="fill">塗り色。</param>
        /// <param name="border">枠色。</param>
        void SetColors(unsigned int background, unsigned int fill, unsigned int border)
        { m_background = background; m_fill = fill; m_border = border; }
        /// <summary>値と最大値から表示比率を設定する。最大値が正でなければ空にする。</summary>
        /// <param name="value">現在値。</param>
        /// <param name="maximum">最大値。</param>
        void SetValue(float value, float maximum)
        { m_ratio = maximum > 0 ? std::clamp(value / maximum, 0.0f, 1.0f) : 0.0f; }
        /// <summary>表示比率を取得する。</summary>
        /// <returns>0〜1の比率。</returns>
        float Ratio() const { return m_ratio; }
        /// <summary>表示中の場合だけ背景・塗り・枠を順に描く。</summary>
        void Draw() override;
    private:
        float m_ratio = 0;
        unsigned int m_background = 0x464646, m_fill = 0xffbe50, m_border = 0xffffff;
    };
}
