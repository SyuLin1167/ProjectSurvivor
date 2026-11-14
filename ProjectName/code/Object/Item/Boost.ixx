export module Object.Boost;
import Object.Player;
import Object.MovingBase;
import Object.Item;

export namespace object
{
    /// <summary>プレイヤーの強化時間を更新する戦利品。</summary>
    struct Boost final : Item
    {
        float Apply(Player& player, MovingBase&) const override { player.BoostAttack(); return 0; }
    };
}
