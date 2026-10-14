module Object.ObjectManager;
import GameSystem.Camera;

namespace object
{
    ObjectManager::ObjectManager()
        : objects()
        , pendingObjects()
        , isUpdate(false)
    {
    }

    ObjectManager::~ObjectManager() = default;

    void ObjectManager::RemoveObject(class GameObject* object)
    {
        ObjectTag tag = object->MyObjectTag();

        //オブジェクトを削除
        auto iter = std::find_if(objects[tag].begin(), objects[tag].end(), [object](const std::shared_ptr<GameObject>& ptr) { return ptr.get() == object; });
        if (iter != objects[tag].end())
        {
            objects[tag].erase(iter);
        }
    }

    void ObjectManager::InitBeforeUpdate()
    {
        //自身に必要なオブジェクトを取得
        for (auto tag : OBJECT_TAGS)
        {
            for (auto& object : objects[tag])
            {
                object->GetReferenceObject([this](uint32_t tag)
                    {
                        return this->GetObjForTag(tag);
                    });
                object->Init();
            }
        }
    }

    void ObjectManager::Update()
    {
        isUpdate = true;
        for (auto tag : OBJECT_TAGS)
        {
            //オブジェクトを更新
            for (auto& object : objects[tag])
            {
                object->UpdateComponents();
                object->Update();
            }
        }

        //当たり判定の更新
        m_colMgr.Step();

        for (auto tag : OBJECT_TAGS)
        {
            for (auto& object : objects[tag])
            {
                object->LateUpdate();
            }
        }
        gameSystem::Camera::Instance().Update();

        // 更新終了後の処理
        isUpdate = false;
        RefreshObjects();
    }

    void ObjectManager::RefreshObjects()
    {
        for (auto tag : OBJECT_TAGS)
        {
            //死亡状態のオブジェクトを削除
            std::erase_if(objects[tag], [](const std::shared_ptr<GameObject>& object)
                {
                    return (object->NowState() & MainState::DEAD) != 0;
                });

            //更新時に追加したオブジェクトを配列に格納
            if (!pendingObjects.empty())
            {
                for (auto pendingObj : pendingObjects)
                {
                    pendingObj->Init();
                    objects[pendingObj->MyObjectTag()].emplace_back(pendingObj);
                }
                pendingObjects.clear();
            }
        }
    }

    std::vector<std::shared_ptr<GameObject>> ObjectManager::GetObjForTag(uint32_t tag)
    {
        if (objects.find(tag) != objects.end())
        {
            return objects[tag];
        }
        return {};
    }

    void ObjectManager::Draw()
    {
        for (auto tag : OBJECT_TAGS)
        {
            //オブジェクトを描画
            for (auto& object : objects[tag])
            {
                object->DrawComponents();
                object->Draw();
            }
        }
    }
}