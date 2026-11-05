export module Object.EnemyConfig;

import MyLib.File.JsonIO;
import MyLib.Math.Vector2;
import <vector>;
import <cmath>;
import <stdexcept>;

export namespace phase2
{
    /// <summary>敵能力の試作設定。JSONの値を検証してから生成・移動・描画へ渡す。</summary>
    struct EnemyConfig
    {
        float Health{};
        float Radius{}; // u
        float Speed{};  // u/s

        /// <summary>DEC-02と#19比較試遊の許可値・速度範囲を確認する。不正時は例外を送出する。</summary>
        /// <param name="data">EnemyConfig.jsonのオブジェクト。欠損・型不正も拒否する。</param>
        /// <returns>検証済みの敵設定。攻撃・接触ダメージの設定は含まない。</returns>
        static EnemyConfig FromJson(const json& data)
        {
            EnemyConfig c;
            c.Health = data.at("health").get<float>();
            c.Radius = data.at("radius").get<float>();
            c.Speed = data.at("speed").get<float>();
            // #19の比較限定でHP20を許可する。基準HP30を維持し、中間値や段階変化は許可しない。
            if ((c.Health != 30 && c.Health != 20) || c.Radius != 16 || !std::isfinite(c.Speed) ||
                c.Speed < 100 || c.Speed > 140)
                throw std::runtime_error("EnemyConfig: values outside DEC-02 / issue 19 approval");
            return c;
        }
    };
}
