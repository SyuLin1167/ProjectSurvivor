module;
#include <DxLib.h>

module MyLib.KeyStatus;

namespace input
{
    KeyStatus::KeyStatus()
    {
        //使用するキーの情報を一通り作成
        for (auto& keyName : KEYS)
        {
            keyData.emplace(keyName, ON_RELEASE);
        }
    }

    void KeyStatus::UpdateKeyStateImpl()
    {
        //キーのステータスを一通り切り替える
        for (auto& key : keyData)
        {
            // 押下中かどうかで場合分け
            if (CheckHitKey(key.first))
            {
                // 押下中の場合
                if (key.second & (ON_RELEASE | RELEASING))
                {
                    key.second = ON_PRESS;
                    continue;
                }
                key.second = PRESSING;
            }
            else
            {
                // 押下されていない場合
                if (key.second & (ON_PRESS | PRESSING))
                {
                    key.second = ON_RELEASE;
                    continue;
                }
                key.second = RELEASING;
            }
        }
    }
}