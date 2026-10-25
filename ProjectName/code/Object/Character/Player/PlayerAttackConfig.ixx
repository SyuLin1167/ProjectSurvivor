export module Object.PlayerAttackConfig;
import MyLib.File.JsonIO;
import <cmath>;
import <stdexcept>;

export namespace phase2
{
    /// <summary>DEC-02の単体自動攻撃設定。数値は試作限定。</summary>
    struct PlayerAttackConfig
    {
        float Range{}; // 中心間距離(u)
        float Damage{};
        float Interval{}; // 秒
        /// <summary>承認値と委任範囲を検証して読み込む。不正な設定は例外にする。</summary>
        /// <param name="data">PlayerAttackConfig.jsonの設定オブジェクト。</param>
        /// <returns>検証済みの攻撃設定。</returns>
        static PlayerAttackConfig FromJson(const json& data)
        {
            PlayerAttackConfig c{data.at("range").get<float>(), data.at("damage").get<float>(), data.at("interval").get<float>()};
            if (!std::isfinite(c.Range) || !std::isfinite(c.Interval) ||
                c.Range < 128 || c.Range > 192 || c.Damage != 10 || c.Interval < 0.4f || c.Interval > 0.8f)
                throw std::runtime_error("PlayerAttackConfig: values outside DEC-02 approval");
            return c;
        }
    };
}
