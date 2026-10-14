module;
#include <DxLib.h>

export module Asset.AssetBase;
import <unordered_map>;
import <string>;
import <chrono>;
import <thread>;
import <string>;

/// <summary>
/// アセット関連
/// </summary>
export namespace asset
{
    /// <summary>
    /// アセットの基底クラス
    /// </summary>
    export class AssetBase
    {
    public:
        /// <summary>
        /// コンストラクタ
        /// </summary>
        AssetBase() = default;

        /// <summary>
        /// デストラクタ
        /// </summary>
        virtual ~AssetBase() = default;

        /// <summary>
        /// ハンドル生成
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        virtual void CreateHandle(std::string handleName, std::string graphName) = 0;

        /// <summary>
        /// ハンドル生成(非同期)
        /// </summary>
        /// <param name="handleName">ハンドル名</param>
        /// <param name="graphName">画像名</param>
        virtual void CreateHandleAsync(std::string handleName, std::string graphName) = 0;

        /// <summary>
        /// ハンドル削除
        /// </summary>
        /// <param name="name">削除するハンドル名</param>
        virtual void DeleteHandle(std::string name) = 0;

    protected:
        /// <summary>
        /// 非同期読み込みの完了待ち
        /// </summary>
        /// <remarks>
        /// CheckHandleASyncLoad の戻り値は 1:読み込み中 / 0:完了 / -1:エラー。
        /// 読み込み中の間だけ待機し、エラー(開始失敗の -1 を含む)では待機しない。
        /// </remarks>
        /// <param name="handle">確認するハンドル</param>
        /// <returns>true:読み込み完了 / false:失敗</returns>
        static bool WaitAsyncLoad(int handle)
        {
            if (handle == -1)
            {
                return false;
            }

            int result = CheckHandleASyncLoad(handle);
            while (result == TRUE)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                result = CheckHandleASyncLoad(handle);
            }
            return result == FALSE;
        }
    };
}
