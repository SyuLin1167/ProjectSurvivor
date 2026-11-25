module;
#include <json.hpp>

export module Object.PlayerData;
import MyLib.File.JsonValue;

import MyLib.Math.Vector2;
import AppContext;

using json = nlohmann::json;
using namespace gameSystem;
using namespace math;

export namespace object
{
    export struct PlayerData
    {
        void Input(const json& data)
        {
            size.x = file::jsonValue::Read<float>(data, "sizeX");
            size.y = file::jsonValue::Read<float>(data, "sizeY");
            pos.x = file::jsonValue::Read<float>(data, "posX");
            pos.y = file::jsonValue::Read<float>(data, "posY");
            moveSpeed.x = file::jsonValue::Read<float>(data, "moveSpeedX");
            moveSpeed.y = file::jsonValue::Read<float>(data, "moveSpeedY");
        }

        Vector2f size;
        Vector2f pos;
        Vector2f moveSpeed;
    };
}
