# Simple Pitch Shift

DAW 向けのピッチシフトのオーディオプラグインです。話し声や歌声の高さと声質を、リアルタイムに変えられます。Windows x64 と Linux x86_64 向けに、VST® 3 と CLAP の形式で配布します。

[OBS Studio 向けのシンプルピッチシフト](https://github.com/AllegroMoltoV/simple-pitchshift)の信号処理を DAW に移植したものです。

A real-time pitch and formant shifter for DAWs, in VST® 3 and CLAP formats for Windows x64 and Linux x86_64.

音のプラグイン倉庫くんの[掲載ページ](https://audio-soko-kun.allegromoltov.jp/plugins/allegromoltov/simple-pitchshift-audio)から、検査を通った版を入手できます。 / Listed on [Audio Plugin Soko-kun](https://audio-soko-kun.allegromoltov.jp/plugins/allegromoltov/simple-pitchshift-audio?lang=en), which links only to releases that passed its checks.

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="docs/images/VST_Compatible_Logo_Steinberg_negative.png">
  <img src="docs/images/VST_Compatible_Logo_Steinberg.png" alt="VST Compatible" width="120">
</picture>

## 機能

- ピッチを `-12` から `+12` 半音まで変えられます。
- フォルマントを `-12` から `+12` 半音まで変えられます。声の高さを保ったまま声質を変えられます。
- ゲインを `-30` から `+30` dB まで変えられます。ピッチ処理の後に適用します。
- 処理窓を 20 ms、30 ms、40 ms から選べます。短いほど追従が速く、長いほど滑らかです。既定値は 40 ms です。
- レイテンシーはサンプルレートごとに一定です。処理窓を変えても、ピッチを 0 にしても変わらないため、DAW の遅延補正がずれません。
- ステレオ音声に対応します。サンプルレートは 44.1 kHz、48 kHz、96 kHz で動作を確かめています。
- 通信しません。音声やデータを外部へ送りません。

## 動作環境

- Windows 10 / 11 x64
- Linux x86_64
- VST 3 または CLAP に対応した DAW

macOS 向けには配布しません。macOS 向けの配布には、Apple の Developer ID の署名と公証が要るためです。

Linux 向けの版は、CI の pluginval と clap-validator で検証しています。Linux の DAW での動作は確かめていません。

## インストール

1. DAW を終了します。
2. [GitHub Releases](https://github.com/AllegroMoltoV/simple-pitchshift-audio/releases) から、環境に合う zip をダウンロードして展開します。Windows では `simple-pitchshift-<版>-windows-x64.zip`、Linux では `simple-pitchshift-<版>-linux-x86_64.zip` です。
3. Windows では、`VST3\Simple Pitch Shift.vst3` フォルダーを、次のどちらかへコピーします。
   - 自分だけが使う場合: `%LOCALAPPDATA%\Programs\Common\VST3\`
   - すべてのユーザーが使う場合: `C:\Program Files\Common Files\VST3\`。管理者の権限が必要です。
4. Windows の CLAP に対応した DAW では、`CLAP\Simple Pitch Shift.clap` を `%LOCALAPPDATA%\Programs\Common\CLAP\` か `C:\Program Files\Common Files\CLAP\` へコピーします。
5. Linux では、`VST3/Simple Pitch Shift.vst3` フォルダーを `~/.vst3/` へ、`CLAP/Simple Pitch Shift.clap` を `~/.clap/` へコピーします。
6. DAW を起動し、必要ならプラグインを再スキャンします。

インストーラーはありません。

## ダウンロードしたファイルの確かめ方

リリースのファイルには、GitHub Actions でビルドしたことを示す証明 (artifact attestation) を付けています。[GitHub CLI](https://cli.github.com/) で確かめられます。

```sh
gh attestation verify simple-pitchshift-<版>-<環境>.zip --repo AllegroMoltoV/simple-pitchshift-audio
```

リリースは不変 (immutable release) に設定しており、公開後にファイルを差し替えられません。

## ビルド

CMake 3.21 以降と C++17 のコンパイラーが必要です。Windows では Visual Studio 2022 以降を使い、CMake はインストールされている中で最も新しい Visual Studio を選びます。Linux では GCC を使います。依存するライブラリと SDK は、構成の時点でコミットを固定して取得します。

```sh
# Windows
cmake -B build -A x64
# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Release

cmake --build build --config Release
ctest --test-dir build -C Release
```

CI では、単体テストに加えて、[pluginval](https://github.com/Tracktion/pluginval) と [clap-validator](https://github.com/free-audio/clap-validator) で検証します。pluginval は VST 3 SDK の validator を内蔵しており、あわせて実行します。

## ライセンス

MIT License です。[LICENSE](LICENSE) を参照してください。

`docs/images/` の VST Compatible ロゴの画像は Steinberg Media Technologies GmbH の商標で、MIT License の対象外です。

配布するファイルに含む第三者のソフトウェアと、そのライセンスは次のとおりです。いずれも配布する zip の `THIRD_PARTY_LICENSES` にライセンス文を同梱しています。

| ソフトウェア | 用途 | ライセンス |
| --- | --- | --- |
| [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) | ピッチとフォルマントの処理 | MIT |
| [Signalsmith Linear](https://github.com/Signalsmith-Audio/linear) | Signalsmith Stretch の依存 | MIT |
| [CLAP](https://github.com/free-audio/clap) | CLAP の形式 | MIT |
| [clap-wrapper](https://github.com/free-audio/clap-wrapper) | CLAP から VST 3 の生成 | MIT |
| [{fmt}](https://github.com/fmtlib/fmt) | clap-wrapper の依存 | MIT |
| [VST 3 SDK](https://github.com/steinbergmedia/vst3sdk) | VST 3 の形式 | MIT |

VST is a registered trademark of Steinberg Media Technologies GmbH.

## AI 利用開示

このプラグインのソースコード、ワークフロー、文書、ロゴは Anthropic Claude Code で作成しました。要件の決定、ロゴのレビュー、外部への操作の承認は作者が行いました。

The source code, workflow, documentation, and logo of this plugin were written with Anthropic Claude Code. The author decided the requirements, reviewed the logo, and approved each external operation.

## 更新履歴

### 1.1.0

- Linux x86_64 向けの VST 3 と CLAP を足しました。CI で pluginval と clap-validator の検証を通しています。Windows 向けの版の信号処理とファイルの配置は 1.0.1 から変わりません。

### 1.0.1

- 音のプラグイン倉庫くんの宣言ファイル `.audio-soko-kun/config.json` (表示名とカテゴリ) とロゴ `.audio-soko-kun/logo.png` を足しました。信号処理は 1.0.0 から変わりません。

### 1.0.0

- 最初の版です。OBS Studio 向けの版の信号処理を、VST 3 と CLAP に移植しました。
