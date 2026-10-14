export module UI.Button;

import <memory>;
import <string>;
import <functional>;
import UI.UIBase;
import MyLib.Math.Vector2;
import MyLib.Shape.Rect;

using namespace math;

export namespace ui
{
    /// <summary>
    /// ボタン定義情報
    /// </summary>
    export struct ButtonDef
    {
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="_text">ボタンテキスト</param>
        /// <param name="_buttonRect">ボタンの形状</param>
        /// <param name="_color">ボタンの色</param>
        ButtonDef(const std::string& _text, const shape::Rect& _buttonRect,const uint32_t _color)
            :text(_text)
            , buttonRect(_buttonRect)
            , color(_color)
        {}

        std::string text;                   // ボタンテキスト
        shape::Rect buttonRect;             // ボタンの形状
        uint32_t color;                     // ボタンの色
        std::function<void()> onHover;      // ホバー時イベント
        std::function<void()> onClicked;    // クリック時イベント
        std::function<void()> onReleased;   // ボタンクリック後イベント
    };

    /// <summary>
    /// ボタンの機能を担当
    /// </summary>
    export class Button : public UIBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="def">ボタンの定義</param>
        Button(const ButtonDef* def);

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~Button();

        /// <summary>
        /// テキストの設定
        /// </summary>
        /// <param name="text">設定するテキスト</param>
        void SetText(const std::string& text)
        {
            m_def->text = text;
        }

        /// <summary>
        /// 更新処理
        /// </summary>
        void Update() override;

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw() override;

        /// <summary>
        /// ホバー時イベント追加
        /// </summary>
        /// <param name="hoverEvent">追加するイベント</param>
        void SetOnMouseHover(std::function<void()> hoverEvent)
        {
            m_def->onHover = std::move(hoverEvent);
        }

        /// <summary>
        /// クリック時イベント
        /// </summary>
        /// <param name="clickEvent">追加するイベント</param>
        void SetOnClicked(std::function<void()> clickEvent)
        {
            m_def->onClicked = std::move(clickEvent);
        }

        /// <summary>
        /// クリック解放時イベント
        /// </summary>
        /// <param name="releaseEvent">追加するイベント</param>
        void SetOnReleased(std::function<void()> releaseEvent)
        {
            m_def->onReleased = std::move(releaseEvent);
        }

    private:
        /// <summary>
        /// クリック時処理
        /// </summary>
        void OnClick();

        /// <summary>
        /// クリック解放時処理
        /// </summary>
        void OnReleased();

        bool m_onClicked{ false }; 
        std::unique_ptr<ButtonDef> m_def;   // ボタン定義
    };
}
