module;
#include <DxLib.h>

export module Object.Goal;

import Object.GameObject;
import MyLib.Math.Vector2;
import Asset.Graph;
import GameSystem.Camera;

using namespace math;
using namespace gameSystem;

export namespace object
{
    export class Goal final : public GameObject
    {
    public:
        Goal(bool& cleared)
            : id()
            , m_cleared(cleared)
        {
            // 画像読み込み
            //AppCtx::AssetMgr().LoadAsync<asset::Graph>("goal", "goal.png");
            m_pos = { 2100.0f, 1000.0f }; // 仮のゴール位置
        }

        ~Goal()
        {
            //AppCtx::AssetMgr().DeleteHandle<asset::Graph>("goal");
            if (auto* mgr = ColMgrPtr())
            {
                mgr->Destroy(id);
            }
        }

        void Init() override
        {
            // コライダー作成
            col2d::ColliderDef def{};
            def.type = col2d::Type::STATIC;
            def.localPos = m_pos;
            def.isTrigger = true;
            def.isActive = true;
            id = ColMgr().CreateCircleCollider(&def, 16.0f, MyObjectTag());
            ColMgr().AddMask(id, col2d::RECT, ObjectTag::PLAYER);

            // 衝突イベント登録
            col2d::ContactListener listener;
            listener.when = [&](const col2d::ContactInfo&) { return true; };
            listener.event = [&]() { m_cleared = true; };
            ColMgr().AddEvent(id, col2d::MakeKey(col2d::RECT, ObjectTag::PLAYER), listener);
        }

        void Update() override {}
        void LateUpdate() override {}

        void Draw() override
        {
            const auto sp = Camera::Instance().WorldToScreen(m_pos);
            DrawCircle((int)sp.x, (int)sp.y, 20, GetColor(255, 215, 0), TRUE);
            DrawFormatString((int)sp.x - 40, (int)sp.y - 40, GetColor(255,255,255), "GOAL");
        }

        ObjectTag MyObjectTag() const override
        {
            return ObjectTag::GOAL;
        }

    private:
        col2d::ColliderID id;
        bool& m_cleared;
    };
}
