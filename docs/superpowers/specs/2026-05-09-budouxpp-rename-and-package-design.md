# budouxpp Rename and CMake Package Design

## 背景

このリポジトリは現在 `budouxpp` というプロジェクト名を持ちながら、公開ライブラリの CMake ターゲット名は `budoux`、実行ファイルは `budouxpp` になっている。利用者から見ると「ライブラリ名」「実行ファイル名」「リポジトリ名」がずれており、`find_package()` で導入するための install/export も未整備である。

今回の変更では公開名を `budouxpp` に統一し、実行ファイルはサンプル用途を明確にするため `example` に改名する。要求される配布形態は GUI インストーラーではなく、CMake から `find_package(budouxpp CONFIG REQUIRED)` で導入できるインストール済みパッケージである。

## ゴール

- 公開ライブラリ名を `budouxpp` に統一する
- `find_package(budouxpp CONFIG REQUIRED)` で利用できる CMake package config を提供する
- 現在 `budouxpp` という名前の executable を `example` に変更する
- README と利用例を新しい公開名と導入方法に合わせる

## 非ゴール

- GUI ベースの OS ネイティブインストーラーを作り込むこと
- ライブラリの API や分割アルゴリズムを変更すること
- BudouX 互換 JSON フォーマットやヘッダ配置を変更すること
- 旧 `budoux` パッケージ名との後方互換 alias を追加すること

## 変更対象の構成

### 1. 公開ライブラリ

- `add_library(budoux INTERFACE)` を `add_library(budouxpp INTERFACE)` に変更する
- install/export 後の利用者向けターゲット名は `budouxpp::budouxpp` とする
- インクルードパスは既存の `#include "budoux/model.hpp"` と `#include "budoux/parser_view.hpp"` を維持し、ヘッダディレクトリ `budoux/` を install する
- `target_include_directories` は `BUILD_INTERFACE` と `INSTALL_INTERFACE` を使って relocatable にする
- Glaze は interface link dependency として引き続き公開する
- `budouxppConfig.cmake` では `CMakeFindDependencyMacro` を使って `find_dependency(glaze CONFIG REQUIRED)` を実行し、その後に export 済み targets を読み込む
- ライブラリの公開要件としては README に合わせて `cxx_std_20` を固定で伝播する

### 2. サンプル実行ファイル

- `add_executable(budouxpp main.cpp)` を `add_executable(example main.cpp)` に変更する
- サンプルは `ENABLE_EXAMPLE` option で有効化し、既定値は `ON` とする
- `main.cpp` 内のロガー名や説明文で、必要なら `example` / `budouxpp` の役割が分かる表現に合わせる
- executable は公開 API ではなくサンプルとして扱う
- `example` は build tree でのみ提供し、install/package の公開成果物には含めない
- `quill` は `example` 専用依存として扱い、公開パッケージの必須依存には含めない
- `example` の private な言語要件は現行どおり compiler に応じた高い標準を許容してよいが、公開ライブラリ要件へは波及させない
- Quill が不要な `find_package` 用パッケージを作れるよう、`ENABLE_EXAMPLE=OFF` なら `find_package(quill)` も不要にする

### 3. CMake package config

以下を追加する:

- `GNUInstallDirs`
- `CMakePackageConfigHelpers`
- `project(budouxpp VERSION 0.1.0 LANGUAGES CXX)` のように package version の供給元を明示する
- `install(TARGETS budouxpp EXPORT budouxppTargets)`
- `install(DIRECTORY budoux/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/budoux)`
- `install(EXPORT budouxppTargets NAMESPACE budouxpp:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/budouxpp)`
- `configure_package_config_file(...)`
- `write_basic_package_version_file(... COMPATIBILITY SameMinorVersion)` を使い、`0.1.x` 系の範囲だけを互換とみなす
- `install(FILES ... budouxppConfig.cmake budouxppConfigVersion.cmake DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/budouxpp)`

`budouxppConfig.cmake` は `glaze` 依存を先に解決してから export 済みターゲットを読み込む薄い設定ファイルにする。header-only library なので import 後に利用者が `target_link_libraries(app PRIVATE budouxpp::budouxpp)` できれば十分である。

## インストール後の見え方

インストールツリーは概ね次の形にする:

```text
<prefix>/
  include/
    budoux/
      model.hpp
      parser_view.hpp
  lib/
    cmake/
      budouxpp/
        budouxppConfig.cmake
        budouxppConfigVersion.cmake
        budouxppTargets.cmake
```

今回は `find_package` 向けライブラリ配布に集中するため、install ツリーには `example` を含めない。

## 利用者フロー

利用側 CMake は次の形を想定する:

```cmake
find_package(budouxpp CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE budouxpp::budouxpp)
```

既存のヘッダ include は維持されるため、利用者コード側では C++ ソースの include 文を変更する必要はない。

## README 更新方針

- リポジトリ名・ライブラリ名の説明を `budouxpp` に統一する
- 埋め込み利用の説明に加えて `find_package(budouxpp CONFIG REQUIRED)` の導入例を載せる
- サンプル実行ファイルの名称を `example` に更新する
- ライブラリ利用の最小要件は C++20、`example` は開発用サンプルとして別要件になりうることを明記する
- C++ の `budoux::` 名前空間と `#include "budoux/..."` は維持されることを明記する
- 開発用コマンドは既存の `./build.sh` と `./test.sh` を維持する

## エラー処理と互換性

- install/export の生成物が不足して `find_package` が失敗する状態は許容しない
- 旧ターゲット名 `budoux` を残さないため、CMake 利用者にとっては明示的な breaking change になる
- 一方でヘッダパスとライブラリの実体は維持するため、C++ ソースコードへの影響は最小に留める

## テスト戦略

実装時は以下を確認する:

1. 既存の build/test が通ること
2. install を実行して package config が生成されること
3. 別の最小 CMake プロジェクトから `find_package(budouxpp CONFIG REQUIRED)` と `target_link_libraries(... budouxpp::budouxpp)` で configure/build できること
4. README の記述が実際の install/export 名と一致すること
5. downstream 側が C++20 で configure/build でき、`example` 用の `quill` を要求されないこと
6. `-DENABLE_EXAMPLE=OFF` で configure した場合に Quill なしでも install/export 検証ができること

## 実装メモ

- 新規に `cmake/budouxppConfig.cmake.in` のようなテンプレートを追加するのが最も単純
- 実装の必須条件は `find_package` 対応であり、CPack や GUI installer の整備は今回のスコープ外とする
