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
        float PlayerCenterX{}; // 初期中心X(u)。コライダーは左上座標へ変換する
        float PlayerCenterY{}; // 初期中心Y(u)
        int GridSize{};        // 表示グリッドの間隔(u)
        math::Vector2f BaseSize{}; // 仮拠点の幅・高さ(u)
        float BaseSpeed{};     // Pとは独立した前進速度(u/s)
        float BaseCenterX{};   // 拠点の初期中心X(u)
        float BaseCenterY{};   // 拠点の初期中心Y(u)
        float TrialEndX{};     // 試行終了の中心X(u)。正式なクリア条件ではない

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
            c.PlayerCenterX = data.at("playerCenterX").get<float>();
            c.PlayerCenterY = data.at("playerCenterY").get<float>();
            if (!data.at("gridSize").is_number_integer())
                throw std::runtime_error("Phase1Config: gridSize must be an integer");
            c.GridSize = data.at("gridSize").get<int>();
            c.BaseSize.x = data.at("baseWidth").get<float>();
            c.BaseSize.y = data.at("baseHeight").get<float>();
            c.BaseSpeed = data.at("baseSpeed").get<float>();
            c.BaseCenterX = data.at("baseCenterX").get<float>();
            c.BaseCenterY = data.at("baseCenterY").get<float>();
            c.TrialEndX = data.at("trialEndX").get<float>();

            // 欠損・型不正はat/getで検出。非有限値、ゼロ間隔、境界外の初期配置も拒否する。
            for (float value : { c.WorldWidth, c.WorldHeight, c.PlayerSize,
                                c.PlayerSpeed, c.PlayerCenterX, c.PlayerCenterY,
                                c.BaseSize.x, c.BaseSize.y, c.BaseSpeed, c.BaseCenterX,
                                c.BaseCenterY, c.TrialEndX })
                if (!std::isfinite(value))
                    throw std::runtime_error("Phase1Config: non-finite value");
            const float half = c.PlayerSize / 2;
            if (c.WorldWidth <= 0 || c.WorldHeight <= 0 || c.PlayerSize <= 0 ||
                c.PlayerSpeed <= 0 || c.GridSize <= 0 ||
                c.PlayerCenterX < half || c.PlayerCenterX > c.WorldWidth - half ||
                c.PlayerCenterY < half || c.PlayerCenterY > c.WorldHeight - half)
                throw std::runtime_error("Phase1Config: invalid dimensions, speed or initial position");
            // 委任された速度範囲と比率を同時に確認。JSON編集で承認範囲を誤って越えない。
            if (c.PlayerSpeed < 180 || c.PlayerSpeed > 300 ||
                c.BaseSpeed < 60 || c.BaseSpeed > 100 ||
                c.PlayerSpeed / c.BaseSpeed < 2 || c.PlayerSpeed / c.BaseSpeed > 3)
                throw std::runtime_error("Phase1Config: speeds outside DEC-01 tuning range");
            if (c.BaseSize.x <= 0 || c.BaseSize.y <= 0 ||
                c.BaseCenterX < c.BaseSize.x / 2 ||
                c.BaseCenterX > c.WorldWidth - c.BaseSize.x / 2 ||
                c.BaseCenterY < c.BaseSize.y / 2 ||
                c.BaseCenterY > c.WorldHeight - c.BaseSize.y / 2 ||
                c.TrialEndX <= c.BaseCenterX || c.TrialEndX > c.WorldWidth - c.BaseSize.x / 2)
                throw std::runtime_error("Phase1Config: invalid base rectangle or trial end");
            return c;
        }
    };
}
