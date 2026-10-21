export module Object.MovingBase;
import Object.GameObject;
import Phase1.Config;
import Object.Player;

export namespace object
{
    class MovingBase final : public GameObject
    {
    public:
        explicit MovingBase(bool& trialFinished);
        ~MovingBase();
        void GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj) override;
        void Init() override;
        void Update() override;
        void LateUpdate() override;
        void Draw() override;
        ObjectTag MyObjectTag() const override { return ObjectTag::BASE; }
    private:
        std::weak_ptr<Player> m_player; // ObjectManager所有。参照で寿命を延ばさない。
        phase1::Config m_config{}; // Phase 1試作設定（u / u/s）
        col2d::ColliderID m_id{};
        bool& m_trialFinished; // 所属Playが所有。正式な勝利フラグとは別
    };
}
