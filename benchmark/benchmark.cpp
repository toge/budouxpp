#include <algorithm>
#include <chrono>
#include <filesystem>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "budoux/model.hpp"
#include "budoux/parser_view.hpp"

namespace fs = std::filesystem;
using my_clock = std::chrono::high_resolution_clock;

static constexpr auto kIterations = 1000;
static constexpr auto kWarmup = 100;

static auto const kText =
  "昨日の夕方、駅前の新しいカフェに行ってみました。"
  "店内は落ち着いた雰囲気で、ゆっくりとコーヒーを楽しめました。"
  "特に抹茶ラテが絶品で、また行きたいと思います。"
  "来週は別のメニューも試してみようと考えています。"
  "友達にもこのカフェのことを教えてあげよう。"
  "駅から徒歩3分の場所にあるので、アクセスも便利です。"
  "営業時間は朝7時から夜10時までで、 Wi-Fiも完備されています。"
  "テラス席もあるので、天気の良い日は外で飲むのもおすすめです。"
  "ただ、週末はかなり混雑するので、時間に余裕を持って行く必要があります。"
  "私はいつも開店直後の7時に行くようにしています。"
  "そうすれば、ゆったりと過ごせるからです。"
  "このカフェの一番のお気に入りは、季節限定のスイーツメニューです。"
  "春には桜フレーバーのケーキ、夏にはマンゴーパフェ、"
  "秋には栗のモンブラン、冬には苺のショートケーキが登場します。"
  "どのスイーツも見た目が美しく、写真を撮るのも楽しみの一つです。"
  "先日は抹茶とホワイトチョコレートのガトーショコラをいただきました。"
  "濃厚でありながら甘すぎず、コーヒーとの相性が抜群でした。"
  "店員さんの接客も丁寧で、居心地の良い空間が広がっています。"
  "また、このカフェでは定期的にワークショップも開催されているそうです。"
  "コーヒーの淹れ方教室や、ラテアート体験など、"
  "コーヒー好きにはたまらないイベントが盛りだくさんです。"
  "今度の週末にはラテアート体験に参加してみようと思います。"
  "楽しみでなりません。"
  "その他にも、読書会や小さなコンサートなど、"
  "様々なカルチャーイベントが開かれています。"
  "このように、単なるカフェではなく、地域のコミュニティスペースとしても"
  "機能しているところが、このカフェの素晴らしい点だと思います。"
  "これからも通い続けたい素敵なお店です。";

struct Stats {
  double min{};
  double median{};
  double max{};
};

template <typename F>
auto measure(F&& f, int n) -> std::vector<double> {
  std::vector<double> samples;
  samples.reserve(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    auto start = my_clock::now();
    f();
    auto end = my_clock::now();
    auto us =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    samples.push_back(static_cast<double>(us));
  }
  return samples;
}

auto compute_stats(std::vector<double> samples) -> Stats {
  std::ranges::sort(samples);
  auto const n = samples.size();
  return {
    .min = samples.front(),
    .median = (n % 2 == 0)
                ? (samples[n / 2 - 1] + samples[n / 2]) / 2.0
                : samples[n / 2],
    .max = samples.back(),
  };
}

auto main(int argc, char* argv[]) -> int {
  auto model_path = fs::path{argv[0]}.parent_path() / ".." / ".." / "data" / "ja.json";
  if (argc >= 2) {
    model_path = fs::path{argv[1]};
  }

  budoux::DefaultModel model{};
  auto const ec = glz::read_file_json(model, model_path.string(), std::string{});
  if (ec) {
    std::println(stderr, "Failed to load model: {}", model_path.string());
    return 1;
  }

  auto const input = std::string_view{kText};
  auto const byte_count = input.size();

  // count segments
  auto segment_count = 0;
  {
    auto tmp = input | budoux::parse(model);
    auto it = tmp.begin();
    auto end = tmp.end();
    while (it != end) {
      ++segment_count;
      ++it;
    }
  }

  std::println("Input: {} bytes, {} segments", byte_count, segment_count);

  // warmup
  std::println("Warmup ({} iterations)...", kWarmup);
  for (int i = 0; i < kWarmup; ++i) {
    for ([[maybe_unused]] auto const _ : input | budoux::parse(model)) {
    }
  }

  // benchmark
  std::println("Benchmark ({} iterations)...", kIterations);
  auto samples = measure(
    [&] {
      for ([[maybe_unused]] auto const _ : input | budoux::parse(model)) {
      }
    },
    kIterations);

  auto stats = compute_stats(std::move(samples));
  auto const us_per_iter = stats.median;
  auto const bytes_per_sec =
    static_cast<double>(byte_count) / (us_per_iter / 1'000'000.0);
  auto const segs_per_sec =
    static_cast<double>(segment_count) / (us_per_iter / 1'000'000.0);

  std::println("");
  std::println("=== Results ({} iterations) ===", kIterations);
  std::println("  Min:     {:8.1f} us", stats.min);
  std::println("  Median:  {:8.1f} us", stats.median);
  std::println("  Max:     {:8.1f} us", stats.max);
  std::println("  Throughput: {:8.0f} bytes/sec  {:8.0f} segs/sec",
               bytes_per_sec, segs_per_sec);

  return 0;
}
