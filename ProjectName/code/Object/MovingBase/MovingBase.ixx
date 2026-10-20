export module Object.MovingBase;
import Object.GameObject;
import Phase1.Config;

export namespace object
{
    class MovingBase final : public GameObject
    {
    public:
        explicit MovingBase(bool& trialFinished);
        ~MovingBase();
        void Init() override;
        void Update() override;
        void LateUpdate() override;
        void Draw() override;
        ObjectTag MyObjectTag() const override { return ObjectTag::BASE; }
    private:
        phase1::Config m_config{}; // Phase 1試作設定（u / u/s）
        col2d::ColliderID m_id{};
        bool& m_trialFinished; // 所属Playが所有。正式な勝利フラグとは別
    };
}
