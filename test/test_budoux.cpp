#include "catch2/catch_all.hpp"

#include <filesystem>
#include <ranges>
#include <string_view>
#include <vector>

namespace budoux {
class DefaultModel;
}

#if __has_include("budoux/model.hpp")
#include "budoux/model.hpp"
#endif

#if __has_include("budoux/parser_view.hpp")
#include "budoux/parser_view.hpp"
#endif

#if __has_include("budoux/model.hpp")
auto constexpr has_model_header = true;
#else
auto constexpr has_model_header = false;
#endif

#if __has_include("budoux/parser_view.hpp")
auto constexpr has_parser_header = true;
#else
auto constexpr has_parser_header = false;
#endif

TEST_CASE("budoux public headers are available") {
  REQUIRE(has_model_header);
  REQUIRE(has_parser_header);
}

TEST_CASE("DefaultModel reads BudouX JSON shape with glaze") {
#if __has_include("budoux/model.hpp")
  auto const model_path =
    std::filesystem::path{__FILE__}.parent_path() / "data" / "mini_model.json";

  budoux::DefaultModel model{};
  auto const ec = glz::read_file_json(model, model_path.string(), std::string{});
  REQUIRE(!ec);

  REQUIRE(model.UW3.contains("は"));
  REQUIRE(model.UW3.at("は") == 10);
  REQUIRE(model.BW2.empty());
#else
  FAIL("budoux/model.hpp is missing");
#endif
}

TEST_CASE("parser_view segments UTF-8 text lazily into string_view slices") {
#if __has_include("budoux/model.hpp") && __has_include("budoux/parser_view.hpp")
  auto const model_path =
    std::filesystem::path{__FILE__}.parent_path() / "data" / "mini_model.json";

  budoux::DefaultModel model{};
  auto const ec = glz::read_file_json(model, model_path.string(), std::string{});
  REQUIRE(!ec);

  auto constexpr input = std::string_view{"今日は天気です。"};
  auto segments = input | budoux::parse(model);
  auto collected = std::vector<std::string_view>{};
  for (auto const segment : segments) {
    collected.push_back(segment);
  }

  REQUIRE(collected == std::vector<std::string_view>{"今日は", "天気です。"});
  REQUIRE(collected.front().data() == input.data());
#else
  FAIL("budoux parser API is missing");
#endif
}

TEST_CASE("parser_view cooperates with standard range adaptors") {
#if __has_include("budoux/model.hpp") && __has_include("budoux/parser_view.hpp")
  auto const model_path =
    std::filesystem::path{__FILE__}.parent_path() / "data" / "mini_model.json";

  budoux::DefaultModel model{};
  auto const ec = glz::read_file_json(model, model_path.string(), std::string{});
  REQUIRE(!ec);

  auto constexpr input = std::string_view{"AあBいC"};
  auto filtered = input | budoux::parse(model) |
                  std::views::filter([](std::string_view piece) {
                    return piece.size() > 1;
                  }) |
                  std::views::take(1);

  auto collected = std::vector<std::string_view>{};
  for (auto const segment : filtered) {
    collected.push_back(segment);
  }
  REQUIRE(collected == std::vector<std::string_view>{"Aあ"});
#else
  FAIL("budoux parser pipeline API is missing");
#endif
}
