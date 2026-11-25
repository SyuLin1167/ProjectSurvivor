export module MyLib.File.JsonValue;
import MyLib.File.JsonIO;
import <optional>;
import <string>;
import <stdexcept>;

export namespace file::jsonValue
{
    /// <summary>JSON要素を指定型へ変換する。変換失敗には項目名を付け、代替値へ補正しない。</summary>
    /// <typeparam name="T">JSONライブラリで取得できる型。数値の範囲制約は呼出し側が検証する。</typeparam>
    /// <param name="value">キー検索済みの値、または配列要素。</param>
    /// <param name="name">エラーに表示する項目名。</param>
    /// <returns>変換した値。不正な型はruntime_error。</returns>
    template<class T>
    T Convert(const json& value, const std::string& name)
    {
        try { return value.get<T>(); }
        catch (const json::exception& error)
        {
            throw std::runtime_error("JSON " + name + ": " + error.what());
        }
    }

    /// <summary>必須項目を参照する。欠落を項目名付きで通知し、JSONを変更しない。</summary>
    /// <param name="data">読込対象オブジェクト。</param>
    /// <param name="key">必須キー。</param>
    /// <returns>dataが存続する間有効な要素参照。</returns>
    inline const json& Member(const json& data, const std::string& key)
    {
        try { return data.at(key); }
        catch (const json::exception& error)
        {
            throw std::runtime_error("JSON " + key + ": " + error.what());
        }
    }

    /// <summary>必須項目を型付きで取得する。検索と変換の失敗を共通の境界で扱う。</summary>
    /// <typeparam name="T">取得する値の型。</typeparam>
    /// <param name="data">読込対象オブジェクト。</param>
    /// <param name="key">必須キー。</param>
    /// <returns>取得値。欠落・型不正はruntime_error。</returns>
    template<class T>
    T Read(const json& data, const std::string& key) { return Convert<T>(Member(data, key), key); }

    /// <summary>省略可能な項目を取得する。nullや不正型は「省略」とみなさない。</summary>
    /// <typeparam name="T">取得する値の型。</typeparam>
    /// <param name="data">読込対象オブジェクト。</param>
    /// <param name="key">省略可能キー。</param>
    /// <returns>キーがない場合のみnullopt、それ以外は取得値。</returns>
    template<class T>
    std::optional<T> ReadOptional(const json& data, const std::string& key)
    {
        // containsだけでは非オブジェクトも「欠落」になるため、入力の形をここで保証する。
        if (!data.is_object()) throw std::runtime_error("JSON " + key + ": expected object");
        const auto found = data.find(key);
        if (found == data.end()) return std::nullopt;
        return Convert<T>(*found, key);
    }

    /// <summary>キーの省略時だけ既定値を採用する。存在する不正値は握りつぶさない。</summary>
    /// <typeparam name="T">取得値と既定値の型。</typeparam>
    /// <param name="data">読込対象オブジェクト。</param>
    /// <param name="key">省略可能キー。</param>
    /// <param name="fallback">既存仕様で定めた省略時の値。</param>
    /// <returns>取得値、または省略時の既定値。</returns>
    template<class T>
    T ReadOr(const json& data, const std::string& key, T fallback)
    {
        return ReadOptional<T>(data, key).value_or(fallback);
    }

    /// <summary>取得値を指定条件で検証し、違反時は項目名と条件を通知する。</summary>
    /// <param name="value">検証対象値。</param>
    /// <param name="key">項目名。</param>
    /// <param name="predicate">正常値ならtrueを返す副作用のない判定。</param>
    /// <param name="rule">満たすべき条件の説明。</param>
    template<class T, class Predicate>
    void Validate(const T& value, const std::string& key, Predicate predicate, const std::string& rule)
    {
        if (!predicate(value)) throw std::runtime_error("JSON " + key + ": " + rule);
    }

    /// <summary>必須値の取得と正常性検証を一つの入力境界で行う。</summary>
    /// <param name="data">読込対象。</param>
    /// <param name="key">必須キー。</param>
    /// <param name="predicate">値の正常性条件。</param>
    /// <param name="rule">条件違反時の説明。</param>
    /// <returns>検証済みの値。</returns>
    template<class T, class Predicate>
    T Read(const json& data, const std::string& key, Predicate predicate, const std::string& rule)
    {
        auto value = Read<T>(data, key);
        Validate(value, key, predicate, rule);
        return value;
    }

    /// <summary>省略可能値が存在する場合だけ正常性を検証する。</summary>
    /// <param name="data">読込対象。</param>
    /// <param name="key">省略可能キー。</param>
    /// <param name="predicate">値の正常性条件。</param>
    /// <param name="rule">条件違反時の説明。</param>
    /// <returns>検証済みの値、または省略時のnullopt。</returns>
    template<class T, class Predicate>
    std::optional<T> ReadOptional(const json& data, const std::string& key, Predicate predicate, const std::string& rule)
    {
        auto value = ReadOptional<T>(data, key);
        if (value) Validate(*value, key, predicate, rule);
        return value;
    }

    /// <summary>空文字を許さない項目で共用する正常性条件。</summary>
    /// <param name="value">取得文字列。</param>
    /// <returns>一文字以上ならtrue。</returns>
    inline bool NonEmpty(const std::string& value) { return !value.empty(); }
}
