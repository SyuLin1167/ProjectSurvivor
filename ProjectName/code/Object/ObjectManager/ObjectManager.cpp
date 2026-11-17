module Object.ObjectManager;
import GameSystem.Camera;
import <stdexcept>;

namespace object
{
    ObjectManager::ObjectManager()
        : m_objects()
        , m_pendingObjects()
        , m_isUpdating(false)
    {
    }

    ObjectManager::~ObjectManager() = default;

    void ObjectManager::Attach(const std::shared_ptr<GameObject>& object)
    {
        if (!object) throw std::invalid_argument("Cannot attach an empty object");
        object->SetColliderManager(&m_colMgr);
        object->OnAttach();
    }

    void ObjectManager::Initialize(GameObject& object)
    {
        object.GetReferenceObject([this](uint32_t tag) { return GetObjForTag(tag); });
        object.Init();
    }

    void ObjectManager::QueueOrRegister(std::shared_ptr<GameObject> object)
    {
        Attach(object);
        if (m_isUpdating) m_pendingObjects.emplace_back(std::move(object));
        else m_objects[object->MyObjectTag()].emplace_back(std::move(object));
    }

    void ObjectManager::AddInitializedObject(std::shared_ptr<GameObject> object)
    {
        if (m_isUpdating) throw std::logic_error("Dynamic addition requires an idle manager");
        Attach(object);
        Initialize(*object);
        // 初期化成功後に公開する。失敗時も所有参照がプールの解放方法を保持する。
        const auto tag = object->MyObjectTag();
        m_objects[tag].emplace_back(std::move(object));
    }

    void ObjectManager::RemoveObject(class GameObject* object)
    {
        ObjectTag tag = object->MyObjectTag();

        //オブジェクトを削除
        auto iter = std::find_if(m_objects[tag].begin(), m_objects[tag].end(), [object](const std::shared_ptr<GameObject>& ptr) { return ptr.get() == object; });
        if (iter != m_objects[tag].end())
        {
            m_objects[tag].erase(iter);
        }
    }

    void ObjectManager::InitBeforeUpdate()
    {
        //自身に必要なオブジェクトを取得
        for (auto tag : OBJECT_TAGS)
        {
            for (auto& object : m_objects[tag])
            {
                Initialize(*object);
            }
        }
    }

    void ObjectManager::Update()
    {
        m_isUpdating = true;
        for (auto tag : OBJECT_TAGS)
        {
            //オブジェクトを更新
            for (auto& object : m_objects[tag])
            {
                object->UpdateComponents();
                object->Update();
            }
        }

        //当たり判定の更新
        m_colMgr.Step();

        for (auto tag : OBJECT_TAGS)
        {
            for (auto& object : m_objects[tag])
            {
                object->LateUpdate();
            }
        }
        gameSystem::Camera::Instance().Update();

        // 更新終了後の処理
        m_isUpdating = false;
        RefreshObjects();
    }

    void ObjectManager::RefreshObjects()
    {
        for (auto tag : OBJECT_TAGS)
        {
            //死亡状態のオブジェクトを削除
            std::erase_if(m_objects[tag], [](const std::shared_ptr<GameObject>& object)
                {
                    return (object->NowState() & MainState::DEAD) != 0;
                });

        }

        // 保留分を切り離し、初期化が失敗した場合も未公開の個体を正しい方法で解放する。
        auto pending = std::move(m_pendingObjects);
        m_pendingObjects.clear();
        for (auto& object : pending)
        {
            Initialize(*object);
            m_objects[object->MyObjectTag()].emplace_back(std::move(object));
        }
    }

    std::vector<std::shared_ptr<GameObject>> ObjectManager::GetObjForTag(uint32_t tag)
    {
        if (m_objects.find(tag) != m_objects.end())
        {
            return m_objects[tag];
        }
        return {};
    }

    void ObjectManager::Draw()
    {
        for (auto tag : OBJECT_TAGS)
        {
            //オブジェクトを描画
            for (auto& object : m_objects[tag])
            {
                object->DrawComponents();
                object->Draw();
            }
        }
    }
}