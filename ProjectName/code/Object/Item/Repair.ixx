export module Object.Repair;
import Object.Player;
import Object.MovingBase;
import Object.Item;

export namespace object
{
    /// <summary>拠点が満HPならフィールドに残る修理品。</summary>
    struct Repair final : Item
    {
        float Apply(Player&, MovingBase& base) const override
        {
            const float healed = base.Repair();
            return healed > 0 ? healed : -1;
        }
    };
}
