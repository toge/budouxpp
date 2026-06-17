#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

#include "glaze/glaze.hpp"
#include "ankerl/unordered_dense.h"

namespace budoux {

/**
 * @brief `std::string_view` を含む異種キー検索に対応したハッシュ関数
 */
struct transparent_string_hash {
  using is_transparent = void;

  /**
   * @brief 文字列ビューをハッシュ化
   * @param value UTF-8 の特徴量キー
   * @return ハッシュ値
   */
  [[nodiscard]]
  auto operator()(std::string_view value) const noexcept {
    return ankerl::unordered_dense::hash<std::string_view>{}(value);
  }

  /**
   * @brief 所有する文字列をハッシュ化
   * @param value UTF-8 の特徴量キー
   * @return ハッシュ値
   */
  [[nodiscard]]
  auto operator()(std::string const& value) const noexcept {
    return (*this)(std::string_view{value});
  }

  /**
   * @brief C 文字列をハッシュ化
   * @param value UTF-8 の特徴量キー
   * @return ハッシュ値
   */
  [[nodiscard]]
  auto operator()(char const* value) const noexcept {
    return (*this)(std::string_view{value});
  }
};

/**
 * @brief UTF-8 の特徴量キーから重みを引くためのテーブル
 */
using score_map =
  ankerl::unordered_dense::map<std::string, int, transparent_string_hash, std::equal_to<>>;

/**
 * @brief BudouX の全特徴量グループを参照で束ねたビュー
 */
struct score_groups_ref {
  score_map const& UW1;  ///< 特徴量グループ `UW1` の重み表です。
  score_map const& UW2;  ///< 特徴量グループ `UW2` の重み表です。
  score_map const& UW3;  ///< 特徴量グループ `UW3` の重み表です。
  score_map const& UW4;  ///< 特徴量グループ `UW4` の重み表です。
  score_map const& UW5;  ///< 特徴量グループ `UW5` の重み表です。
  score_map const& UW6;  ///< 特徴量グループ `UW6` の重み表です。
  score_map const& BW1;  ///< 特徴量グループ `BW1` の重み表です。
  score_map const& BW2;  ///< 特徴量グループ `BW2` の重み表です。
  score_map const& BW3;  ///< 特徴量グループ `BW3` の重み表です。
  score_map const& TW1;  ///< 特徴量グループ `TW1` の重み表です。
  score_map const& TW2;  ///< 特徴量グループ `TW2` の重み表です。
  score_map const& TW3;  ///< 特徴量グループ `TW3` の重み表です。
  score_map const& TW4;  ///< 特徴量グループ `TW4` の重み表です。
};

/**
 * @brief BudouX 互換モデルに必要な特徴量グループ API を表すコンセプト
 * @tparam T モデル型
 */
template <typename T>
concept ModelConcept = requires(T const model, std::string_view key) {
  { model.scores().UW1.contains(key) } -> std::convertible_to<bool>;
  { model.scores().UW2.contains(key) } -> std::convertible_to<bool>;
  { model.scores().UW3.contains(key) } -> std::convertible_to<bool>;
  { model.scores().UW4.contains(key) } -> std::convertible_to<bool>;
  { model.scores().UW5.contains(key) } -> std::convertible_to<bool>;
  { model.scores().UW6.contains(key) } -> std::convertible_to<bool>;
  { model.scores().BW1.contains(key) } -> std::convertible_to<bool>;
  { model.scores().BW2.contains(key) } -> std::convertible_to<bool>;
  { model.scores().BW3.contains(key) } -> std::convertible_to<bool>;
  { model.scores().TW1.contains(key) } -> std::convertible_to<bool>;
  { model.scores().TW2.contains(key) } -> std::convertible_to<bool>;
  { model.scores().TW3.contains(key) } -> std::convertible_to<bool>;
  { model.scores().TW4.contains(key) } -> std::convertible_to<bool>;
};

namespace detail {

/**
 * @brief 重み表に含まれるすべての重みを合計
 * @param map 重み表
 * @return 重みの合計値
 */
[[nodiscard]] inline
auto sum_group(score_map const& map) noexcept {
  auto total = 0LL;
  for (auto const& entry : map) {
    total += entry.second;
  }
  return total;
}

/**
 * @brief 特徴量グループから BudouX の基準スコアを計算
 * @param scores 特徴量グループ群
 * @return すべての境界判定で使う基準スコア
 */
[[nodiscard]] inline
auto compute_base_score(score_groups_ref const& scores) noexcept {
  // すべての特徴量グループに含まれる重みを合算する
  auto const total = sum_group(scores.UW1) + sum_group(scores.UW2) +
                     sum_group(scores.UW3) + sum_group(scores.UW4) +
                     sum_group(scores.UW5) + sum_group(scores.UW6) +
                     sum_group(scores.BW1) + sum_group(scores.BW2) +
                     sum_group(scores.BW3) + sum_group(scores.TW1) +
                     sum_group(scores.TW2) + sum_group(scores.TW3) +
                     sum_group(scores.TW4);
  // BudouX では総和の -0.5 倍を基準スコアとして使う
  return -0.5 * static_cast<double>(total);
}

}  // namespace detail

/**
 * @brief 公式 `ja.json` と同じレイアウトを持つ既定の BudouX モデル
 */
struct DefaultModel {
  score_map UW1{};  ///< 特徴量グループ `UW1` の重み表
  score_map UW2{};  ///< 特徴量グループ `UW2` の重み表
  score_map UW3{};  ///< 特徴量グループ `UW3` の重み表
  score_map UW4{};  ///< 特徴量グループ `UW4` の重み表
  score_map UW5{};  ///< 特徴量グループ `UW5` の重み表
  score_map UW6{};  ///< 特徴量グループ `UW6` の重み表
  score_map BW1{};  ///< 特徴量グループ `BW1` の重み表
  score_map BW2{};  ///< 特徴量グループ `BW2` の重み表
  score_map BW3{};  ///< 特徴量グループ `BW3` の重み表
  score_map TW1{};  ///< 特徴量グループ `TW1` の重み表
  score_map TW2{};  ///< 特徴量グループ `TW2` の重み表
  score_map TW3{};  ///< 特徴量グループ `TW3` の重み表
  score_map TW4{};  ///< 特徴量グループ `TW4` の重み表

  /**
   * @brief すべての特徴量グループを参照として返す
   * @return 特徴量グループへの参照ビュー
   */
  [[nodiscard]]
  auto scores() const noexcept -> score_groups_ref {
    return {UW1, UW2, UW3, UW4, UW5, UW6, BW1,
            BW2, BW3, TW1, TW2, TW3, TW4};
  }

  /**
   * @brief このモデルの BudouX 基準スコアを返す
   * @return キャッシュ済みの基準スコア
   */
  [[nodiscard]]
  auto base_score() const noexcept -> double {
    if (!cached_base_score_.has_value()) {
      // 初回だけ特徴量グループから基準スコアを計算して保持する
      cached_base_score_ = detail::compute_base_score(scores());
    }
    // 2 回目以降は計算済みの値をそのまま返す
    return *cached_base_score_;
  }

private:
  mutable std::optional<double> cached_base_score_{};
};

/**
 * @brief 任意の BudouX 互換モデルから基準スコアを取得する
 * @tparam Model BudouX 互換モデル型
 * @param model モデル実体
 * @return 基準スコア
 */
template <ModelConcept Model>
[[nodiscard]] inline
auto model_base_score(Model const& model) noexcept {
  if constexpr (requires { { model.base_score() } -> std::convertible_to<double>; }) {
    // モデル側がキャッシュ済みスコアを提供する場合はそれを優先する
    return static_cast<double>(model.base_score());
  }
  // それ以外のモデルでは特徴量グループから毎回計算する
  return detail::compute_base_score(model.scores());
}

}  // namespace budoux

/**
 * @brief `budoux::DefaultModel` と JSON キーを結び付ける Glaze 用メタデータ
 */
template <>
struct glz::meta<budoux::DefaultModel> {
  using T = budoux::DefaultModel;
  static constexpr auto value = glz::object("UW1", &T::UW1, "UW2", &T::UW2,
                                            "UW3", &T::UW3, "UW4", &T::UW4,
                                            "UW5", &T::UW5, "UW6", &T::UW6,
                                            "BW1", &T::BW1, "BW2", &T::BW2,
                                            "BW3", &T::BW3, "TW1", &T::TW1,
                                            "TW2", &T::TW2, "TW3", &T::TW3,
                                            "TW4", &T::TW4);
};
