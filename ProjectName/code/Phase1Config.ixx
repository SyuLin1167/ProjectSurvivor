module;
#include <json.hpp>
#include <cmath>
#include <stdexcept>

export module Phase1.Config;

export namespace phase1
{
    // DEC-01の試作値。JSONを正本とし、未読込時の値をゲームへ流さない。
    struct Config
    {
        float WorldWidth{};    // マップ横幅(u)
        float WorldHeight{};   // マップ縦幅(u)
        float PlayerSize{};    // 正方形プレイヤーの一辺(u)
        float PlayerSpeed{};   // 移動速度(u/s)
        float PlayerCenterX{}; // 初期中心X(u)。コライダーは左上座標へ変換する
        float PlayerCenterY{}; // 初期中心Y(u)
        int GridSize{};        // 表示グリッドの間隔(u)

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

            // 欠損・型不正はat/getで検出。非有限値、ゼロ間隔、境界外の初期配置も拒否する。
            for (float value : { c.WorldWidth, c.WorldHeight, c.PlayerSize,
                                c.PlayerSpeed, c.PlayerCenterX, c.PlayerCenterY })
                if (!std::isfinite(value))
                    throw std::runtime_error("Phase1Config: non-finite value");
            const float half = c.PlayerSize / 2;
            if (c.WorldWidth <= 0 || c.WorldHeight <= 0 || c.PlayerSize <= 0 ||
                c.PlayerSpeed <= 0 || c.GridSize <= 0 ||
                c.PlayerCenterX < half || c.PlayerCenterX > c.WorldWidth - half ||
                c.PlayerCenterY < half || c.PlayerCenterY > c.WorldHeight - half)
                throw std::runtime_error("Phase1Config: invalid dimensions, speed or initial position");
            return c;
        }
    };
}
