module;
#include <json.hpp>
#include <cmath>
#include <stdexcept>

export module Phase1.Config;
import MyLib.Math.Vector2;

export namespace phase1
{
    /// <summary>DEC-01の試作値。JSONを正本とし、未読込の値をゲームへ流さない。</summary>
    struct Config
    {
        float WorldWidth{};    // マップ横幅(u)
        float WorldHeight{};   // マップ縦幅(u)
        float PlayerSize{};    // 正方形プレイヤーの一辺(u)
        float PlayerSpeed{};   // 移動速度(u/s)
        math::Vector2f PlayerCenter{}; // 初期中心(u)。コライダーは左上座標へ変換する
        int GridSize{};        // 表示グリッドの間隔(u)
        math::Vector2f BaseSize{}; // 仮拠点の幅・高さ(u)
        float BaseSpeed{};     // Pとは独立した前進速度(u/s)
        math::Vector2f BaseCenter{}; // 拠点の初期中心(u)
        float TrialEndX{};     // DEC-03試作の成功終端X(u)。既存JSONキーを維持

        /// <summary>JSONを読み込み、寸法・初期位置・承認された速度範囲を検証する。不正時は例外を送出する。</summary>
        /// <param name="data">Phase1Config.jsonの設定オブジェクト。</param>
        /// <returns>承認範囲と寸法・位置を検証済みの試作設定。</returns>
        static Config FromJson(const nlohmann::json& data)
        {
            Config c;
            c.WorldWidth = data.at("worldWidth").get<float>();
            c.WorldHeight = data.at("worldHeight").get<float>();
            c.PlayerSize = data.at("playerSize").get<float>();
            c.PlayerSpeed = data.at("playerSpeed").get<float>();
            c.PlayerCenter.x = data.at("playerCenterX").get<float>();
            c.PlayerCenter.y = data.at("playerCenterY").get<float>();
            if (!data.at("gridSize").is_number_integer())
                throw std::runtime_error("Phase1Config: gridSize must be an integer");
            c.GridSize = data.at("gridSize").get<int>();
            c.BaseSize.x = data.at("baseWidth").get<float>();
            c.BaseSize.y = data.at("baseHeight").get<float>();
            c.BaseSpeed = data.at("baseSpeed").get<float>();
            c.BaseCenter.x = data.at("baseCenterX").get<float>();
            c.BaseCenter.y = data.at("baseCenterY").get<float>();
            c.TrialEndX = data.at("trialEndX").get<float>();

            // 欠損・型不正はat/getで検出。非有限値、ゼロ間隔、境界外の初期配置も拒否する。
            for (float value : { c.WorldWidth, c.WorldHeight, c.PlayerSize,
                                c.PlayerSpeed, c.PlayerCenter.x, c.PlayerCenter.y,
                                c.BaseSize.x, c.BaseSize.y, c.BaseSpeed, c.BaseCenter.x,
                                c.BaseCenter.y, c.TrialEndX })
                if (!std::isfinite(value))
                    throw std::runtime_error("Phase1Config: non-finite value");
            const float half = c.PlayerSize / 2;
            if (c.WorldWidth <= 0 || c.WorldHeight <= 0 || c.PlayerSize <= 0 ||
                c.PlayerSpeed <= 0 || c.GridSize <= 0 ||
                c.PlayerCenter.x < half || c.PlayerCenter.x > c.WorldWidth - half ||
                c.PlayerCenter.y < half || c.PlayerCenter.y > c.WorldHeight - half)
                throw std::runtime_error("Phase1Config: invalid dimensions, speed or initial position");
            // DEC-03: Phase 1の調整範囲を継承せず、P/拠点速度は固定する。
            if (c.PlayerSpeed != 240 || c.BaseSpeed != 80)
                throw std::runtime_error("Phase1Config: speeds must match DEC-03 fixed values");
            if (c.BaseSize.x <= 0 || c.BaseSize.y <= 0 ||
                c.BaseCenter.x < c.BaseSize.x / 2 ||
                c.BaseCenter.x > c.WorldWidth - c.BaseSize.x / 2 ||
                c.BaseCenter.y < c.BaseSize.y / 2 ||
                c.BaseCenter.y > c.WorldHeight - c.BaseSize.y / 2 ||
                c.TrialEndX <= c.BaseCenter.x || c.TrialEndX > c.WorldWidth - c.BaseSize.x / 2)
                throw std::runtime_error("Phase1Config: invalid base rectangle or trial end");
            // DEC-03の委任範囲。設定名は維持し、未承認の短縮・延長を拒否する。
            const float distance = c.TrialEndX - c.BaseCenter.x;
            if (distance < 4800 || distance > 7200)
                throw std::runtime_error("Phase1Config: distance outside DEC-03 range");
            return c;
        }
    };
}
