#pragma once

#include <array>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string_view>
#include <utility>

#include "budoux/model.hpp"

namespace budoux {

namespace detail {

/**
 * @brief 指定したバイトが UTF-8 の継続バイトかどうかを返す
 * @param byte 判定対象のバイト値
 * @return 継続バイトなら `true`
 */
[[nodiscard]] inline
auto is_continuation_byte(unsigned char byte) noexcept {
  return (byte & 0b1100'0000U) == 0b1000'0000U;
}

/**
 * @brief UTF-8 の先頭バイトから期待されるコードポイント長を返す
 * @param byte 先頭バイト
 * @return 想定されるバイト長。不正な先頭バイトは `1` を返す
 */
[[nodiscard]] inline
auto utf8_length(unsigned char byte) noexcept {
  if ((byte & 0b1000'0000U) == 0U) {
    return 1;
  }
  if ((byte & 0b1110'0000U) == 0b1100'0000U) {
    return 2;
  }
  if ((byte & 0b1111'0000U) == 0b1110'0000U) {
    return 3;
  }
  if ((byte & 0b1111'1000U) == 0b1111'0000U) {
    return 4;
  }
  return 1;
}

/**
 * @brief UTF-8 コードポイントの継続バイト列が妥当か検証する
 * @param input 入力テキスト
 * @param offset 現在のバイトオフセット
 * @param length 想定しているコードポイント長
 * @return 構造的に妥当な UTF-8 列なら `true`
 */
[[nodiscard]] inline auto has_valid_continuations(
  std::string_view input, std::size_t offset, std::size_t length) noexcept {
  if ((offset + length) > input.size()) {
    return false;
  }
  for (auto const index : std::views::iota(std::size_t{1}, length)) {
    if (!is_continuation_byte(
          static_cast<unsigned char>(input[offset + index]))) {
      return false;
    }
  }
  return true;
}

/**
 * @brief 次の UTF-8 コードポイントを元の文字列を参照するビューとして返す
 * @param input 入力テキスト
 * @param offset 現在のバイトオフセット。読み取り後は次位置へ更新される
 * @return 1 コードポイント分のビュー。不正列は 1 バイトとして返す
 */
[[nodiscard]] inline
auto decode_next(std::string_view input, std::size_t& offset) noexcept -> std::string_view {
  // 入力末尾に達していたら空ビューを返します。
  if (offset >= input.size()) {
    return {};
  }

  auto const start = offset;
  auto const lead = static_cast<unsigned char>(input[offset]);
  // 先頭バイトから期待される UTF-8 長を見積もります。
  auto length = utf8_length(lead);

  // 不正な先頭バイトや継続バイト不足は 1 バイト文字として扱います。
  if ((length == 2U && lead < 0xC2U) ||
      !has_valid_continuations(input, offset, length)) {
    length = 1;
  }

  // 次回の読み取り位置へ進め、元文字列を参照するビューを返します。
  offset += length;
  return input.substr(start, length);
}

/**
 * @brief 隣接する2つまでのスライスを1つのビューにまとめる
 * @param first 先頭のスライス
 * @param second 次のスライス
 * @return 連続領域を指す結合済みビュー
 */
[[nodiscard]] inline
auto merge(std::string_view first, std::string_view second) noexcept -> std::string_view {
  if (first.empty()) {
    return second;
  }
  if (second.empty()) {
    return first;
  }
  auto const begin = first.data();
  auto const end = second.data() + second.size();
  return {begin, static_cast<std::size_t>(end - begin)};
}

/**
 * @brief 隣接する 3 つまでのスライスを 1 つのビューにまとめる
 * @param first 先頭のスライス
 * @param second 2 番目のスライス
 * @param third 3 番目のスライス
 * @return 連続領域を指す結合済みビュー
 */
[[nodiscard]] inline
auto merge(std::string_view first, std::string_view second, std::string_view third) noexcept {
  return merge(merge(first, second), third);
}

/**
 * @brief 指定した特徴量キーに対応する重みを返す
 * @param map 重み表
 * @param key 特徴量キー
 * @return 登録済みの重み。未登録なら `0.0` を返す
 */
[[nodiscard]] inline
auto lookup(score_map const& map, std::string_view key) noexcept -> double {
  auto const it = map.find(key);
  if (it == map.end()) {
    return 0.0;
  }
  return static_cast<double>(it->second);
}

}  // namespace detail

/**
 * @brief `std::string_view` の断片を遅延生成するBudouXパーサービュー
 * @tparam Model BudouX 互換モデル型
 */
template <ModelConcept Model>
class parser_view : public std::ranges::view_interface<parser_view<Model>> {
public:
  /**
   * @brief 分割されたテキストを走査する入力イテレータ
   */
  class iterator {
  public:
    using iterator_concept = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::string_view;

    /**
     * @brief 終端イテレータ
     */
    iterator() = default;

    /**
     * @brief パーサービューに対応するイテレータを構築
     * @param parent 親ビュー
     */
    explicit iterator(parser_view const* parent) noexcept : parent_{parent} {
      // 空入力や無効な親は即座に終端状態として扱う
      if ((parent_ == nullptr) || parent_->input_.empty()) {
        finished_ = true;
        return;
      }

      // 最初の境界判定に必要な 6 文字窓を作り、先頭セグメントを確定する
      initialize_window();
      locate_next_boundary();
    }

    /**
     * @brief 現在のセグメントを返す
     * @return 現在位置のセグメントビュー
     */
    [[nodiscard]] auto operator*() const noexcept -> std::string_view {
      return parent_->input_.substr(segment_start_, segment_end_ - segment_start_);
    }

    /**
     * @brief 次のセグメントへ進める
     * @return 自身への参照
     */
    auto operator++() noexcept -> iterator& {
      if (finished_) {
        return *this;
      }

      // 現在セグメントが末尾まで届いていれば次は終端
      if (segment_end_ == parent_->input_.size()) {
        finished_ = true;
        return *this;
      }

      // 次セグメントの開始位置を更新し、次の境界候補を探索する
      segment_start_ = segment_end_;
      advance_boundary();
      locate_next_boundary();
      return *this;
    }

    /**
     * @brief 次のセグメントへ進める
     * @return インクリメント前のイテレータ状態
     */
    auto operator++(int) noexcept -> iterator {
      auto copy = *this;
      ++(*this);
      return copy;
    }

    /**
     * @brief 終端センチネルとの比較を行います。
     * @param self 比較対象のイテレータ
     * @param sentinel 終端センチネル
     * @return 走査が完了していれば `true`
     */
    friend auto operator==(iterator const& self,
                           std::default_sentinel_t sentinel) noexcept -> bool {
      std::ignore = sentinel;
      return self.finished_;
    }

  private:
    /**
     * @brief 最初の境界判定に使うスライディングウィンドウを初期化する
     */
    void initialize_window() noexcept {
      auto offset = std::size_t{0};
      window_[2] = detail::decode_next(parent_->input_, offset);
      window_[3] = detail::decode_next(parent_->input_, offset);
      window_[4] = detail::decode_next(parent_->input_, offset);
      window_[5] = detail::decode_next(parent_->input_, offset);
      next_decode_offset_ = offset;
    }

    /**
     * @brief 境界候補を 1 コードポイント分だけ進める
     */
    void advance_boundary() noexcept {
      window_[0] = window_[1];
      window_[1] = window_[2];
      window_[2] = window_[3];
      window_[3] = window_[4];
      window_[4] = window_[5];
      window_[5] = detail::decode_next(parent_->input_, next_decode_offset_);
    }

    /**
     * @brief 現在の境界候補に対する BudouX スコアを評価する
     * @return 文をここで分割すべきならtrue
     */
    [[nodiscard]] auto should_split() const noexcept -> bool {
      auto const scores = parent_->model_->scores();
      auto score = parent_->base_score_;

      // 単文字特徴量の寄与を加算する
      score += detail::lookup(scores.UW1, window_[0]);
      score += detail::lookup(scores.UW2, window_[1]);
      score += detail::lookup(scores.UW3, window_[2]);
      score += detail::lookup(scores.UW4, window_[3]);
      score += detail::lookup(scores.UW5, window_[4]);
      score += detail::lookup(scores.UW6, window_[5]);

      // 連続する文字列特徴量の寄与を加算する
      score += detail::lookup(scores.BW1, detail::merge(window_[1], window_[2]));
      score += detail::lookup(scores.BW2, detail::merge(window_[2], window_[3]));
      score += detail::lookup(scores.BW3, detail::merge(window_[3], window_[4]));
      score += detail::lookup(scores.TW1,
                              detail::merge(window_[0], window_[1], window_[2]));
      score += detail::lookup(scores.TW2,
                              detail::merge(window_[1], window_[2], window_[3]));
      score += detail::lookup(scores.TW3,
                              detail::merge(window_[2], window_[3], window_[4]));
      score += detail::lookup(scores.TW4,
                              detail::merge(window_[3], window_[4], window_[5]));
      return score >= 0.0;
    }

    /**
     * @brief 現在セグメントの終端バイト位置を探索する
     */
    void locate_next_boundary() noexcept {
      // 判定対象の中心文字がない場合はこれ以上セグメントを作らない
      if (window_[2].empty()) {
        finished_ = true;
        return;
      }

      // 右隣がない場合は残り全体が最後のセグメント
      if (window_[3].empty()) {
        segment_end_ = parent_->input_.size();
        return;
      }

      // 境界候補を 1 文字ずつ進めながら分割点を探す
      while (!window_[3].empty()) {
        segment_end_ = static_cast<std::size_t>(window_[3].data() - parent_->input_.data());
        if (should_split()) {
          return;
        }
        advance_boundary();
      }
      segment_end_ = parent_->input_.size();
    }

    parser_view const* parent_{nullptr};
    std::array<std::string_view, 6> window_{};
    std::size_t next_decode_offset_{0};
    std::size_t segment_start_{0};
    std::size_t segment_end_{0};
    bool finished_{false};
  };

  /**
   * @brief 遅延評価されるパーサービューを構築します。
   * @param input UTF-8 入力テキストです。
   * @param model BudouX モデルです。
   */
  parser_view(std::string_view input, Model const& model) noexcept
      : input_{input}, model_{&model}, base_score_{model_base_score(model)} {}

  /**
   * @brief 先頭セグメントを指すイテレータを返します。
   * @return 先頭イテレータです。
   */
  [[nodiscard]]
  auto begin() const noexcept -> iterator { return iterator{this}; }

  /**
   * @brief 終端センチネルを返します。
   * @return 終端センチネルです。
   */
  [[nodiscard]]
  auto end() const noexcept -> std::default_sentinel_t {
    return std::default_sentinel;
  }

private:
  std::string_view input_{};
  Model const* model_{nullptr};
  double base_score_{0.0};
};

/**
 * @brief BudouX モデル参照を保持するパイプアダプタ
 * @tparam Model BudouX 互換モデル型
 */
template <ModelConcept Model>
class parse_adaptor {
public:
  /**
   * @brief アダプタを構築します。
   * @param model BudouX モデルです。
   */
  explicit parse_adaptor(Model const& model) noexcept : model_{&model} {}

  /**
   * @brief 入力文字列を BudouX で分割するビューへ変換します。
   * @param input UTF-8 入力文です。
   * @return 遅延評価されるパーサービューです。
   */
  [[nodiscard]]
  auto operator()(std::string_view input) const noexcept {
    return parser_view<Model>{input, *model_};
  }

  /**
   * @brief パイプ構文での利用を有効にします。
   * @param input UTF-8 入力文です。
   * @param adaptor パースアダプタです。
   * @return 遅延評価されるパーサービューです。
   */
  friend
  auto operator|(std::string_view input, parse_adaptor const& adaptor) noexcept {
    return adaptor(input);
  }

private:
  Model const* model_;
};

/**
 * @brief パイプ構文向けの BudouX パースアダプタを生成します。
 * @tparam Model BudouX 互換モデル型です。
 * @param model BudouX モデルです。
 * @return パースアダプタです。
 */
template <ModelConcept Model>
[[nodiscard]] inline
auto parse(Model const& model) noexcept {
  return parse_adaptor<Model>{model};
}

}  // namespace budoux
