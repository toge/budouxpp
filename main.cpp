#include <filesystem>
#include <print>
#include <ranges>
#include <string_view>

#include "budoux/model.hpp"
#include "budoux/parser_view.hpp"
#include "quill/Backend.h"
#include "quill/Frontend.h"
#include "quill/LogMacros.h"
#include "quill/Logger.h"
#include "quill/sinks/ConsoleSink.h"

/**
 * @brief Builds the default console logger used by the demo.
 * @return Quill logger pointer.
 */
[[nodiscard]] auto make_logger() -> quill::Logger* {
  quill::Backend::start();
  auto console_sink =
    quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console");
  auto* logger =
    quill::Frontend::create_or_get_logger("example", std::move(console_sink));
  return logger;
}

/**
 * @brief Prints every segment in a range.
 * @tparam Range Range type yielding `std::string_view`.
 * @param title Section title.
 * @param range Range to print.
 */
template <std::ranges::input_range Range>
void print_segments(std::string_view title, Range&& range) {
  std::print("{}\n", title);
  for (auto const segment : range) {
    std::print("  [{}]\n", segment);
  }
}

/**
 * @brief Entry point for the BudouX demo executable.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Process exit code.
 */
auto main(int argc, char const* argv[]) -> int {
  auto* logger = make_logger();

  if (argc < 3) {
    LOG_ERROR(logger, "Usage: {} <model.json> <input>", argv[0]);
    return 1;
  }

  auto const model_path = std::filesystem::path{argv[1]};
  auto const input = std::string_view{argv[2]};

  budoux::DefaultModel model{};
  auto const ec = glz::read_file_json(model, model_path.string(), std::string{});
  if (ec) {
    LOG_ERROR(logger, "Failed to read model: {}", model_path.string());
    return 1;
  }

  LOG_INFO(logger, "Loaded model from {}", model_path.string());
  print_segments("All segments:", input | budoux::parse(model));
  print_segments(
    "Composed view (filter + take):",
    input | budoux::parse(model) |
      std::views::filter([](std::string_view piece) { return piece.size() > 1; }) |
      std::views::take(3));

  return 0;
}
