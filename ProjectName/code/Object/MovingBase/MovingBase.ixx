export module Object.MovingBase;
import Object.GameObject;
import Phase1.Config;

export namespace object
{
    /// <summary>プレイヤーを待たずに直進する試作拠点。勝敗判定はPlayが担当する。</summary>
    class MovingBase final : public GameObject
    {
    public:
        /// <summary>試作設定を読み込み、HP100で生成する。</summary>
        MovingBase();
        /// <summary>所有するコライダーを解放する。</summary>
        ~MovingBase();
        /// <summary>承認された中心座標から拠点矩形とコライダーを初期化する。</summary>
        void Init() override;
        /// <summary>時間に応じた前進量をコライダーへ渡す。</summary>
        void Update() override;
        /// <summary>衝突処理後の座標を読み戻す。</summary>
        void LateUpdate() override;
        /// <summary>可視の拠点矩形を描画する。画面座標のHUDはPlayが担当する。</summary>
        void Draw() override;
        /// <summary>拠点の更新・参照に使用するタグを返す。</summary>
        /// <returns>拠点を識別するObjectTag::BASE。</returns>
        ObjectTag MyObjectTag() const override { return ObjectTag::BASE; }

        /// <summary>追跡対象として現在の中心位置を返す。</summary>
        /// <returns>キャラバン中心のワールド座標（u）。</returns>
        math::Vector2f Center() const { return m_pos + m_config.BaseSize.Half(); }
        /// <summary>試作の被害を下限0で適用する。ゲーム更新の停止はPlayが担当する。</summary>
        /// <param name="damage">正の有限ダメージ。</param>
        /// <returns>有効な拠点のHPを減少させた場合true。</returns>
        bool TakeDamage(float damage);
        /// <summary>防衛の効果を検証するため残HPを返す。</summary>
        /// <returns>0〜100の試作HP。</returns>
        float Health() const { return m_health; }
        /// <summary>開始時に読み込んだ拠点中心Xを返す。</summary>
        /// <returns>開始X（u）。</returns>
        float StartX() const { return m_config.BaseCenter.x; }
        /// <summary>開始時に読み込んだ終端Xを返す。</summary>
        /// <returns>終端X（u）。</returns>
        float EndX() const { return m_config.TrialEndX; }
        /// <summary>表示領域と画面外マーカーの計算に使う寸法を返す。</summary>
        /// <returns>拠点の幅・高さ（u）。</returns>
        math::Vector2f Size() const { return m_config.BaseSize; }
    private:
        float m_health = 100.0f; // DEC-02固定値。Phase 2では調整権限なし。
        phase1::Config m_config{}; // Phase 1試作設定（u / u/s）
        col2d::ColliderID m_id{};
    };
}
