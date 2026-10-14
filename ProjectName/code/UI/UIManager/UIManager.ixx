export module UI.UIManager;

import UI.UIBase;
import MyLib.MouseStatus;
import <unordered_map>;
import <memory>;
import <queue>;
import <vector>;
import <cstdint>;
import <algorithm>;

/// <summary>
/// UI関連
/// </summary>
export namespace ui
{
    /// <summary>
    /// UI要素の識別子
    /// </summary>
    export struct UIElementID
    {
        uint32_t index;          // インデックス
        uint16_t generation;     // 生成世代
    };

    /// <summary>
    /// UI要素管理
    /// </summary>
    export class UIManager
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        UIManager() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        ~UIManager() = default;

        /// <summary>
        /// UI要素を生成
        /// </summary>
        /// <typeparam name="T">生成するUI要素の型</typeparam>
        /// <param name="args">コンストラクタ引数</param>
        /// <returns>生成されたUI要素の識別子</returns>
        template<typename T, typename... Args>
        UIElementID Create(Args&&... args)
        {
            auto element = std::make_unique<T>(std::forward<Args>(args)...);
            UIElementID id = CreateID();
            m_elements[id.index] = std::move(element);
            return id;
        }

        /// <summary>
        /// UI要素を取得
        /// </summary>
        /// <param name="id">検索する要素の識別子</param>
        /// <returns>UI要素の識別子</returns>
        UIBase* GetElement(const UIElementID& id)
        {
            if (!IsValid(id))
            {
                return nullptr;
            }

            if (auto it = m_elements.find(id.index); it != m_elements.end())
            {
                return it->second.get();
            }
            return nullptr;
        }

        /// <summary>
        /// UI要素を破棄
        /// </summary>
        /// <param name="id">UI要素の識別子</param>
        void Destroy(const UIElementID& id);

        /// <summary>
        /// UI要素をクリア(すべての既存IDが無効になる)
        /// </summary>
        void Clear();

        /// <summary>
        /// 更新処理
        /// </summary>
        void Update();

        /// <summary>
        /// 描画処理
        /// </summary>
        void Draw();

    private:
        /// <summary>
        /// UI要素の識別子を生成
        /// </summary>
        /// <returns>生成されたUI要素の識別子</returns>
        UIElementID CreateID();

        /// <summary>
        /// IDスロット(サイズは減らず、採番カウンターを兼ねる)
        /// </summary>
        struct IDSlot
        {
            uint16_t generation = 0;    // 現在の世代(Destroy時にのみ進める)
            bool alive = false;         // 使用中か
        };

        /// <summary>
        /// IDが現在有効か
        /// </summary>
        bool IsValid(const UIElementID& id) const
        {
            return id.index < m_slots.size()
                && m_slots[id.index].alive
                && m_slots[id.index].generation == id.generation;
        }

        /// <summary>
        /// スロットを解放し世代を進める
        /// </summary>
        void ReleaseSlot(uint32_t index);

        std::queue<uint32_t> m_freeIndex; // 再利用可能なindex(重複登録しない)
        std::vector<IDSlot> m_slots;      // index毎の世代と使用状態
        std::unordered_map<uint32_t, std::unique_ptr<UIBase>> m_elements; // UI要素のマップ
    };
}
