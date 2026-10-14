module;
#include <cmath>

export module MyLib.Math.Vector2;

/// <summary>
/// 数学関連
/// </summary>
export namespace math
{
    /// <summary>
    /// 二次元ベクトル
    /// </summary>
    export template <typename T>
    struct Vector2 final
    {
        T x; // x
        T y; // y

        /// <summary>
        /// コンストラクタ
        /// </summary>
        Vector2() = default;

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="_x">入力値X</param>
        /// <param name="_y">入力値Y</param>
        constexpr Vector2(T _x, T _y) : x(_x), y(_y) {}

        /// <summary>
        /// コンストラクタ
        /// </summary>
        /// <param name="_x">入力値X</param>
        /// <param name="_y">入力値Y</param>
        template <typename U>
        constexpr Vector2(U _x, U _y) : x(static_cast<T>(_x)), y(static_cast<T>(_y)) {}

        /// <summary>
        /// 自身の長さ
        /// </summary>
        /// <returns>自身の長さ</returns>
        inline T Length() const
        {
            return static_cast<T>(std::sqrt(x * x + y * y));
        }

        /// <summary>
        /// 自身の長さの二乗
        /// </summary>
        /// <returns>自身の長さの二乗</returns>
        inline T LengthSq() const
        {
            return static_cast<T>(x * x + y * y);
        }

        /// <summary>
        /// 正規化
        /// </summary>
        /// <returns>正規化後の値</returns>
        inline Vector2 Norm() const
        {
            if (T len = Length(); len > 0)
            {
                return { x / len, y / len };
            }
            return Vector2{};
        }

        /// <summary>
        /// 自身との距離
        /// </summary>
        /// <param name="_rhs">対象のベクトル</param>
        /// <returns>自身と対象との距離</returns>
        constexpr T Distance(const Vector2& _rhs) const
        {
            return static_cast<T>(std::sqrt((x - _rhs.x) * (x - _rhs.x) + (y - _rhs.y) * (y - _rhs.y)));
        }

        /// <summary>
        /// 二等分
        /// </summary>
        /// <returns>二等分した値</returns>
        constexpr Vector2<float> Half() const
        {
            return Vector2<float>(static_cast<float>(x) / 2, static_cast<float>(y) / 2);
        }

        /// <summary>
        /// 内積
        /// </summary>
        /// <param name="_rhs">対象のベクトル</param>
        /// <returns>内積</returns>
        inline T Dot(const Vector2& _rhs) const
        {
            return x * _rhs.x + y * _rhs.y;
        }

        /// <summary>
        /// 外積
        /// </summary>
        /// <param name="_rhs">対象のベクトル</param>
        /// <returns>外積</returns>
        inline T Cross(const Vector2& _rhs) const
        {
            return x * _rhs.y - y * _rhs.x;
        }

        /// <summary>
        /// 加算
        /// </summary>
        /// <param name="_rhs">加算ベクトル</param>
        /// <returns>加算後のベクトル</returns>
        constexpr Vector2 operator +(const Vector2& _rhs) const
        {
            return Vector2(x + _rhs.x, y + _rhs.y);
        }

        /// <summary>
        /// 加算
        /// </summary>
        /// <param name="_value">加算値</param>
        /// <returns>加算後のベクトル</returns>
        constexpr  Vector2 operator +(T _value) const
        {
            return Vector2(x + _value, y + _value);
        }

        /// <summary>
        /// 加算
        /// </summary>
        /// <typeparam name="U">他の型</typeparam>
        /// <param name="_value">加算値</param>
        /// <returns>加算後のベクトル</returns>
        template<typename U>
        constexpr Vector2 operator +(const Vector2<U>& _value) const
        {
            return Vector2(x + static_cast<T>(_value.x), y + static_cast<T>(_value.y));
        }

        /// 加算
        /// </summary>
        /// <param name="_rhs">加算ベクトル</param>
        /// <returns>加算後のベクトル</returns>
        constexpr Vector2& operator +=(const Vector2& _rhs)
        {
            x += _rhs.x;
            y += _rhs.y;
            return *this;
        }

        /// <summary>
        /// 加算
        /// </summary>
        /// <param name="_value">加算値</param>
        /// <returns>加算後のベクトル</returns>
        constexpr Vector2& operator +=(float _value)
        {
            x += _value;
            y += _value;
            return *this;
        }

        /// <summary>
        /// 減算
        /// </summary>
        /// <param name="_rhs">減算ベクトル</param>
        /// <returns>減算後のベクトル</returns>
        constexpr Vector2 operator -(const Vector2& _rhs) const
        {
            return Vector2(x - _rhs.x, y - _rhs.y);
        }

        /// <summary>
        /// 減算
        /// </summary>
        /// <param name="_value">減算値</param>
        /// <returns>減算後のベクトル</returns>
        constexpr Vector2 operator -(float _value) const
        {
            return Vector2(x - _value, y - _value);
        }

        /// <summary>
        /// 減算
        /// </summary>
        /// <typeparam name="U">他の型</typeparam>
        /// <param name="_value">減算値</param>
        template<typename U>
        constexpr Vector2 operator -(const Vector2<U>& _value) const
        {
            return Vector2(x - static_cast<T>(_value.x), y - static_cast<T>(_value.y));
        }

        /// <summary>
        /// 減算
        /// </summary>
        /// <param name="_rhs">減算ベクトル</param>
        /// <returns>減算後のベクトル</returns>
        constexpr Vector2& operator -=(const Vector2& _rhs)
        {
            x -= _rhs.x;
            y -= _rhs.y;
            return *this;
        }

        /// <summary>
        /// 減算
        /// </summary>
        /// <param name="_value">減算値</param>
        /// <returns>減算後のベクトル</returns>
        constexpr Vector2& operator -=(T _value)
        {
            x -= _value;
            y -= _value;
            return *this;
        }

        /// <summary>
        /// 乗算
        /// </summary>
        /// <param name="_rhs">乗算ベクトル</param>
        /// <returns>乗算後のベクトル</returns>
        constexpr Vector2 operator *(const Vector2& _rhs) const
        {
            return Vector2(x * _rhs.x, y * _rhs.y);
        }

        /// <summary>
        /// 乗算
        /// </summary>
        /// <param name="_value">乗算値</param>
        /// <returns>乗算後のベクトル</returns>
        constexpr Vector2 operator *(T _value) const
        {
            return Vector2(x * _value, y * _value);
        }

        /// <summary>
        /// 乗算
        /// </summary>
        /// <typeparam name="U">他の型</typeparam>
        /// <param name="_value">乗算値</param>
        /// <returns>乗算後のベクトル</returns>
        template<typename U>
        constexpr Vector2 operator *(const Vector2<U>& _value) const
        {
            return Vector2(x * static_cast<T>(_value.x), y * static_cast<T>(_value.y));
        }

        /// <summary>
        /// 乗算
        /// </summary>
        /// <param name="_rhs">乗算ベクトル</param>
        /// <returns>乗算後のベクトル</returns>
        constexpr Vector2& operator *=(const Vector2& _rhs)
        {
            x *= _rhs.x;
            y *= _rhs.y;
            return *this;
        }

        /// <summary>
        /// 乗算
        /// </summary>
        /// <param name="_value">乗算値</param>
        /// <returns>乗算後のベクトル</returns>
        constexpr Vector2& operator *=(T _value)
        {
            x *= _value;
            y *= _value;
            return *this;
        }

        /// <summary>
        /// 除算
        /// </summary>
        /// <param name="_rhs">除算ベクトル</param>
        /// <returns>除算後のベクトル</returns>
        constexpr Vector2 operator /(const Vector2& _rhs) const
        {
            return Vector2(x / _rhs.x, y / _rhs.y);
        }

        /// <summary>
        /// 除算
        /// </summary>
        /// <param name="_value">除算値</param>
        /// <returns>除算後のベクトル</returns>
        constexpr Vector2 operator /(T _value) const
        {
            return Vector2(x / _value, y / _value);
        }

        /// <summary>
        /// 除算
        /// </summary>
        /// <typeparam name="U">他の型</typeparam>
        /// <param name="_value">除算値</param>
        template<typename U>
        constexpr Vector2 operator /(const Vector2<U>& _value) const
        {
            return Vector2(x / static_cast<T>(_value.x), y / static_cast<T>(_value.y));
        }

        /// <summary>
        /// 除算
        /// </summary>
        /// <param name="_rhs">除算ベクトル</param>
        /// <returns>除算後のベクトル</returns>
        constexpr Vector2& operator /=(const Vector2& _rhs)
        {
            x /= _rhs.x;
            y /= _rhs.y;
            return *this;
        }

        /// <summary>
        /// 除算
        /// </summary>
        /// <param name="_value">除算値</param>
        /// <returns>除算後のベクトル</returns>
        constexpr Vector2& operator /=(T _value)
        {
            x /= _value;
            y /= _value;
            return *this;
        }

        /// <summary>
        /// 代入
        /// </summary>
        /// <param name="_rhs">代入するベクトル</param>
        /// <returns>代入後のベクトル</returns>
        constexpr Vector2 operator =(const Vector2& _rhs)
        {
            x = _rhs.x;
            y = _rhs.y;
            return *this;
        }

        /// <summary>
        /// 代入
        /// </summary>
        /// <param name="_value">代入する値</param>
        /// <returns>代入後のベクトル</returns>
        constexpr Vector2 operator =(T _value)
        {
            x = _value;
            y = _value;
            return *this;
        }

        /// <summary>
        /// 等価比較
        /// </summary>
        /// <param name="_rhs">比較対象のベクトル</param>
        constexpr bool operator ==(const Vector2& _rhs) const
        {
            return (x == _rhs.x && y == _rhs.y);
        }

        /// <summary>
        /// zeroベクトルを取得
        /// </summary>
        /// <returns>zeroベクトル</returns>
        static constexpr Vector2<T> ZERO_VEC() noexcept
        {
            Vector2<T> zero(T(0), T(0));
            return zero;
        };
    };

    using Vector2f = Vector2<float>; // float型の二次元ベクトル
    using Vector2i = Vector2<int>;   // int型の二次元ベクトル
    using Vector2d = Vector2<double>; // double型の二次元ベクトル
    using Vector2u = Vector2<unsigned>; // unsigned型の二次元ベクトル
}
