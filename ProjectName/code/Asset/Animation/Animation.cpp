module;

#include <DxLib.h>

module Asset.Animation;

import Asset.DivisionGraph;
import GameSystem.FrameRate;
import MyLib.Loading.LoadingContext;
import AppContext;

using namespace gameSystem;

namespace asset
{
    Animation::Animation()
        :m_divGraph(std::make_unique<DivisionGraph>())
    {
        //処理なし
    }

    Animation::~Animation()
    {
        m_animations.clear();
    }

    void Animation::CreateHandle(std::string handleName, std::string graphName)
    {
        m_divGraph->CreateHandle(handleName, graphName);
    }

    void Animation::CreateHandle(std::string handleName, std::string graphName, const int& sizeX, const int& sizeY)
    {
        m_divGraph->ChangeDivSize(sizeX, sizeY);
        m_divGraph->CreateHandle(handleName, graphName);
    }

    void Animation::CreateHandleAsync(std::string handleName, std::string graphName)
    {
        m_divGraph->CreateHandleAsync(handleName, graphName);
    }

    void Animation::CreateHandleAsync(std::string handleName, std::string graphName, const int& sizeX, const int& sizeY)
    {
        m_divGraph->ChangeDivSize(sizeX, sizeY);
        m_divGraph->CreateHandleAsync(handleName, graphName);
    }

    void Animation::CreateAnim(const AnimationDef& animDef)
    {
        m_animations[animDef.name] = { m_divGraph->GetHandle(animDef.name), animDef };
        if (animDef.endFrame == 0)
        {
            m_animations[animDef.name].second.endFrame = m_divGraph->GetTotalDiv(animDef.name);
        }
    }

    void Animation::CreateAnimCategory(const std::string& category, const std::vector<AnimationDef> animDefs)
    {
        for (auto& def : animDefs)
        {
            m_animations[def.name] = { m_divGraph->GetHandle(category), def };
        }
    }

    void Animation::DeleteHandle(std::string name)
    {
        m_divGraph->DeleteHandle(name);
    }

    void Animation::DeleteAnim(const std::string& animName)
    {
        m_divGraph->DeleteHandle(animName);
        m_animations.erase(animName);
    }
}