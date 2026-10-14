export module MyLib.Shape.Circle;
import MyLib.Math.Vector2;

using namespace math;

/// <summary>
/// 形状関連
/// </summary>
export namespace shape
{
    /// <summary>
    /// 円
    /// </summary>
    export struct Circle
    {
        Vector2f center; // 中心座標
        float radius; // 半径

        /// <summary>
        /// コンストラクタ
        /// </summary>
        Circle() :center({ 0,0 }), radius(0.0f) {};

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="center">中心座標</param>
        /// <param name="radius">半径</param>
        Circle(const Vector2f& center, const float& radius) : center(center), radius(radius) {};

        /// <summary>
        /// 点が円の内部にあるか判定
        /// </summary>
        /// <param name="point">判定する点</param>
        /// <returns>円の内部にあるかどうか</returns>
        bool IsInside(const Vector2f& point) const
        {
            Vector2f diff = point - center;
            return diff.LengthSq() <= radius * radius;
        }

        /// <summary>
        /// 円同士の重なり判定
        /// </summary>
        /// <param name="other">判定する円</param>
        /// <returns>重なっているかどうか</returns>
        bool IsInside(const Circle& other) const
        {
            Vector2f diff = other.center - center;
            float rSum = radius + other.radius;
            return diff.LengthSq() <= rSum * rSum;
        }
    };
}
