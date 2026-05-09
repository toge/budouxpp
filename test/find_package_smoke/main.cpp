#include <string_view>

#include "budoux/model.hpp"
#include "budoux/parser_view.hpp"

auto main() -> int {
  auto const text = std::string_view{"今日は天気です。"};
  budoux::DefaultModel model{};
  auto const view = text | budoux::parse(model);
  (void)view;
  return 0;
}
