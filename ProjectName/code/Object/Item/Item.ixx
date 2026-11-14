export module Object.Item;
import Object.Player;
import Object.MovingBase;
import Collider.ColliderManager;
import MyLib.Math.Vector2;

export namespace object
{
    /// <summary>固定スロットの戦利品。接触通知だけを記録し、勝敗判定後に効果を適用する。</summary>
    struct Item
    {
        math::Vector2f center{};
        float remaining = 20;
        bool touching{};
        col2d::ColliderManager* manager{};
        col2d::ColliderID id{};
        /// <summary>期限切れ・取得・シーン終了のいずれでも登録を解除する。</summary>
        virtual ~Item() { if (manager) manager->Destroy(id); }
        /// <summary>成功時は回復量（強化は0）、取得不能なら負値を返す。</summary>
        /// <param name="player">取得者。</param>
        /// <param name="base">修理対象。</param>
        /// <returns>取得結果。</returns>
        virtual float Apply(Player& player, MovingBase& base) const = 0;
    };
}
