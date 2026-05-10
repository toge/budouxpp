# budouxpp

budouxpp は、[BudouX](https://github.com/google/budoux) 互換のモデルを使って UTF-8 文字列の改行候補を遅延分割できる C++ ライブラリです。公開される CMake のライブラリ名 / パッケージ名は `budouxpp` ですが、C++ API の名前空間はこれまで通り `budoux` のままです。`budoux::parse(model)` は `std::string_view` の range を返すので、標準の `std::views` とそのまま合成できます。

このリポジトリの `budoux::DefaultModel` は、BudouX 公式リポジトリで配布されている `models/ja.json` と同じキー構成の JSON を読み込めます。

## 特徴

- ヘッダオンリーで利用できます
- 入力文字列をコピーせず `std::string_view` でセグメントを返します
- `input | budoux::parse(model) | std::views::filter(...)` のように Range パイプラインへ自然に組み込めます
- モデルの JSON 読み込みは Glaze と組み合わせてそのまま扱えます

## 必要環境

- C++20 以降（ライブラリ利用時の最小要件）
- Glaze

## 導入方法

インストール済みのパッケージとして使う場合は、`find_package(budouxpp CONFIG REQUIRED)` で設定ファイルを読み込み、公開ターゲット `budouxpp::budouxpp` をリンクしてください。

```cmake
find_package(budouxpp CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE budouxpp::budouxpp)
```

このリポジトリをそのまま組み込む埋め込み利用も引き続き可能です。その場合は、CMake から公開ライブラリ名 `budouxpp` を直接リンクしてください。

```cmake
add_subdirectory(path/to/budouxpp)
target_link_libraries(your_target PRIVATE budouxpp)
```

ビルド用のサンプル実行ファイル名は `example` です。

## 最小例

```cpp
#include <iostream>
#include <string>
#include <string_view>

#include "budoux/model.hpp"
#include "budoux/parser_view.hpp"

int main() {
  budoux::DefaultModel model{};
  auto const ec =
    glz::read_file_json(model, "/absolute/path/to/ja.json", std::string{});
  if (ec) {
    std::cerr << "モデルの読み込みに失敗しました\n";
    return 1;
  }

  auto const input = std::string_view{"今日は天気です。"};
  for (auto const piece : input | budoux::parse(model)) {
    std::cout << '[' << piece << "]\n";
  }
}
```

出力例:

```text
[今日は]
[天気です。]
```

## Range と組み合わせる

`budoux::parse()` の戻り値は view なので、標準の range adaptor をそのまま使えます。

```cpp
auto const input = std::string_view{"AあBいC"};
auto filtered = input | budoux::parse(model) |
                std::views::filter([](std::string_view piece) {
                  return piece.size() > 1;
                }) |
                std::views::take(1);

for (auto const piece : filtered) {
  std::cout << piece << '\n';
}
```

## モデルについて

- `budoux::DefaultModel` は `UW1` 〜 `UW6`, `BW1` 〜 `BW3`, `TW1` 〜 `TW4` を持つ BudouX 互換 JSON を想定しています
- この実装は UTF-8 を前提にしています
- セグメントは入力文字列を参照する `std::string_view` なので、走査中は元文字列を保持してください

## 開発用コマンド

```sh
./build.sh
./test.sh
```
