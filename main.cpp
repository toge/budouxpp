#include <filesystem>
#include <iostream>
#include <ranges>
#include <string_view>

#include "budoux/model.hpp"
#include "budoux/parser_view.hpp"

/**
 * @brief Prints every segment in a range.
 * @tparam Range Range type yielding `std::string_view`.
 * @param title Section title.
 * @param range Range to print.
 */
template <std::ranges::input_range Range>
void print_segments(std::string_view title, Range&& range) {
  std::cout << title << "\n";
  for (auto const segment : range) {
    std::cout << "  [" << segment << "]\n";
  }
}

/**
 * @brief Entry point for the BudouX demo executable.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
auto main(int argc, char const* argv[]) -> int {
  if (argc < 3) {
    std::cerr << "Usage: " << argv[0] << " <model_path> <input_text>\n";
    return 1;
  }

  auto const model_path = std::filesystem::path{argv[1]};
  auto const input = std::string_view{argv[2]};

  budoux::DefaultModel model{};
  auto const ec = glz::read_file_json(model, model_path.string(), std::string{});
  if (ec) {
    std::cerr << "Failed to load model: " << model_path.string() << "\n";
    return 1;
  }

  std::cout << "Loaded model from " << model_path.string() << "\n";
  print_segments("All segments:", input | budoux::parse(model));
  print_segments(
    "Composed view (filter + take):",
    input | budoux::parse(model) |
      std::views::filter([](std::string_view piece) { return piece.size() > 1; }) |
      std::views::take(3));

  return 0;
}
