module;
#include <DxLib.h>

module Object.MovingBase;
import Object.BaseMotion;
import MyLib.File.FileSystem;
import GameSystem.Camera;
import GameSystem.CameraGeometry;
import GameSystem.Window;
import MyLib.Shape.Rect;
import Collider;

namespace object
{
    MovingBase::MovingBase(bool& trialFinished) : m_trialFinished(trialFinished)
    {
        m_config = phase1::Config::FromJson(gameSystem::AppCtx::FileSystem().jsonIO.Load(
            gameSystem::AppCtx::FileSystem().GetDataDir() / "Phase1Config"));
    }

    MovingBase::~MovingBase()
    {
        if (auto* manager = ColMgrPtr()) manager->Destroy(m_id);
    }

    void MovingBase::GetReferenceObject(std::function<std::vector<std::shared_ptr<GameObject>>(uint32_t)> referenceObj)
    {
        const auto players = referenceObj(ObjectTag::PLAYER);
        // Phase 1はプレイヤー1体。空の配列ではfront()を呼ばない。
        m_player = players.empty() ? std::shared_ptr<Player>{} :
            std::dynamic_pointer_cast<Player>(players.front());
    }

    void MovingBase::Init()
    {
        // 承認値は中心、RectColliderは左上。Pと同じ座標契約を使用する。
        const auto size = m_config.BaseSize;
        m_pos = math::Vector2f{m_config.BaseCenterX, m_config.BaseCenterY} - size.Half();
        col2d::ColliderDef def{};
        def.localPos = m_pos;
        def.isActive = true;
        m_id = ColMgr().CreateRectCollider(&def, size, MyObjectTag());
        // マスクなしでPと相互通過。基盤のtrigger/地形経路へ入らない。
    }

    void MovingBase::Update()
    {
        m_velocity = phase1::BaseDisplacement(Center().x,
            m_config.TrialEndX, m_config.BaseSpeed, m_deltaTime);
        ColMgr().GetCollider(m_id)->SetVelocity(m_velocity);
        // 座標はここでは足さず、既存ColliderManager::Stepに一度だけ反映させる。
    }

    void MovingBase::LateUpdate()
    {
        m_pos = ColMgr().GetCollider(m_id)->GetColliderDef()->localPos;
        if (Center().x >= m_config.TrialEndX)
            m_trialFinished = true;
    }

    void MovingBase::Draw()
    {
        const auto size = m_config.BaseSize;
        const auto screen = gameSystem::Window::GetWindowData()->SIZE;
        const shape::Rect viewport{{0, 0}, {static_cast<float>(screen.x), static_cast<float>(screen.y)}};
        const shape::Rect baseRect{gameSystem::Camera::Instance().WorldToScreen(m_pos), size};
        const auto marker = cameraGeometry::BaseMarker(baseRect, viewport);
        // 描画だけを省く。Update/LateUpdateと進行値・距離の表示は画面外でも継続する。
        if (!marker.offscreen)
        {
            DrawBox(static_cast<int>(baseRect.Left()), static_cast<int>(baseRect.Top()),
                static_cast<int>(baseRect.Right()), static_cast<int>(baseRect.Bottom()),
                GetColor(255, 155, 40), true);
            DrawString(static_cast<int>(baseRect.Left() + 8), static_cast<int>(baseRect.Top() + 24),
                "BASE ->", GetColor(0, 0, 0));
        }
        // 画面外でも更新される実座標で進行を観測する。
        DrawFormatString(16, 64, GetColor(255, 190, 80),
            "BASE center: (%.2f, %.2f) u   speed: %.0f u/s   trial end X: %.0f",
            Center().x, Center().y,
            m_config.BaseSpeed, m_config.TrialEndX);

        if (const auto player = m_player.lock())
        {
            const auto center = shape::Rect{m_pos, size}.Center();
            const auto delta = center - player->Center();
            const float distance = delta.Length();
            // 128uはDEC-01の測定目安。勝敗・衝突・拠点速度には使用しない。
            DrawBox(12, 86, 720, 114, GetColor(15, 20, 25), true);
            DrawFormatString(16, 90, GetColor(255, 190, 80),
                "P -> BASE: %.1f u | %s | %s", distance,
                distance <= 128.0f ? "NEAR (<=128u)" : "FAR (>128u)",
                marker.offscreen ? "OFFSCREEN" : "ONSCREEN");
            if (marker.offscreen)
            {
                // 配置は画面端、向きはPからBASEへの実方向。境界clamp中も区別する。
                const auto direction = delta.Norm();
                const math::Vector2f side{-direction.y, direction.x};
                const auto tip = marker.position + direction * 14.0f;
                const auto left = marker.position - direction * 10.0f + side * 8.0f;
                const auto right = marker.position - direction * 10.0f - side * 8.0f;
                DrawTriangle(static_cast<int>(tip.x), static_cast<int>(tip.y),
                    static_cast<int>(left.x), static_cast<int>(left.y),
                    static_cast<int>(right.x), static_cast<int>(right.y), GetColor(255, 155, 40), true);
                DrawString(static_cast<int>(marker.position.x - 20),
                    static_cast<int>(marker.position.y + 18), "BASE", GetColor(255, 190, 80));
            }
        }
    }
}
