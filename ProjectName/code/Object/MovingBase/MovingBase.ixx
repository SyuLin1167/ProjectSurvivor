export module Object.MovingBase;
import Object.GameObject;
import Phase1.Config;
import Object.Player;

export namespace object
{
    /// <summary>プレイヤーを待たずに直進するPhase 1の試作拠点。</summary>
    class MovingBase final : public GameObject
    {
    public:
        /// <summary>試作設定を読み込み、所属シーンの試行終了フラグを参照する。</summary>
        /// <param name="trialFinished">終端到達時にtrueへ設定する所属シーンのフラグ。拠点より長く生存すること。</param>
        explicit MovingBase(bool& trialFinished);
        /// <summary>所有するコライダーを解放する。</summary>
        ~MovingBase();
        /// <summary>距離表示用に単一プレイヤーへの弱参照を取得する。</summary>
        /// <param name="referenceObj">タグに一致するオブジェクト一覧を取得する関数。</param>
        void GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj) override;
        /// <summary>承認された中心座標から拠点矩形とコライダーを初期化する。</summary>
        void Init() override;
        /// <summary>時間に応じた前進量をコライダーへ渡す。</summary>
        void Update() override;
        /// <summary>衝突処理後の座標を読み戻し、試行終端を判定する。</summary>
        void LateUpdate() override;
        /// <summary>可視の拠点と、方向・距離の試作表示を描画する。</summary>
        void Draw() override;
        /// <summary>拠点の更新・参照に使用するタグを返す。</summary>
        /// <returns>拠点を識別するObjectTag::BASE。</returns>
        ObjectTag MyObjectTag() const override { return ObjectTag::BASE; }

        /// <summary>追跡対象として現在の中心位置を返す。</summary>
        /// <returns>キャラバン中心のワールド座標（u）。</returns>
        math::Vector2f Center() const { return m_pos + m_config.BaseSize.Half(); }
        /// <summary>試作の被害を下限0で適用する。HP0でも移動・表示は継続する。</summary>
        /// <param name="damage">正の有限ダメージ。</param>
        /// <returns>有効な拠点のHPを減少させた場合true。</returns>
        bool TakeDamage(float damage);
        /// <summary>防衛の効果を検証するため残HPを返す。</summary>
        /// <returns>0〜100の試作HP。</returns>
        float Health() const { return m_health; }
    private:
        float m_health = 100.0f; // DEC-02固定値。Phase 2では調整権限なし。
        std::weak_ptr<Player> m_player; // ObjectManager所有。参照で寿命を延ばさない。
        phase1::Config m_config{}; // Phase 1試作設定（u / u/s）
        col2d::ColliderID m_id{};
        bool& m_trialFinished; // 所属Playが所有。正式な勝利フラグとは別
    };
}
