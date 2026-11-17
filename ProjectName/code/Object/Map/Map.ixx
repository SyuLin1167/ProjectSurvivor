export module Object.Map;

import Scene.StageConfig;

import Object.GameObject;
/// <summary>
/// オブジェクト関連
/// </summary>
export namespace object
{
    /// <summary>
    /// ステージの寸法をカメラに設定し、可視範囲の地面を描く
    /// </summary>
    export class Map final :public GameObject
    {
    public:
        /// @brief 検証済みの寸法を使って見下ろしマップを構築する。
        /// @param config この試行の設定。
        explicit Map(const stage::StageConfig& config);

        /// <summary>
        /// 初期化処理
        /// </summary>
        void Init() override;

        /// <summary>
        /// 描画
        /// </summary>
        void Draw() override;

        /// <summary>
        /// 自身のタグ
        /// </summary>
        /// <returns>Map</returns>
        ObjectTag MyObjectTag() const override
        {
            return ObjectTag::MAP;
        }

    private:
        stage::StageConfig m_stageConfig;
    };
}
