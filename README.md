# Simple Pitch Shift

DAW 向けのピッチシフトのオーディオプラグインです。話し声や歌声の高さと声質を、リアルタイムに変えられます。Windows x64 向けに、VST® 3 と CLAP の形式で配布します。

[OBS Studio 向けのシンプルピッチシフト](https://github.com/AllegroMoltoV/simple-pitchshift)の信号処理を DAW に移植したものです。

A real-time pitch and formant shifter for DAWs, in VST® 3 and CLAP formats for Windows x64.

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
- VST 3 または CLAP に対応した DAW

macOS と Linux 向けには配布しません。

## インストール

1. DAW を終了します。
2. [GitHub Releases](https://github.com/AllegroMoltoV/simple-pitchshift-audio/releases) から `simple-pitchshift-<版>-windows-x64.zip` をダウンロードして展開します。
3. `VST3\Simple Pitch Shift.vst3` フォルダーを、次のどちらかへコピーします。
   - 自分だけが使う場合: `%LOCALAPPDATA%\Programs\Common\VST3\`
   - すべてのユーザーが使う場合: `C:\Program Files\Common Files\VST3\`。管理者の権限が必要です。
4. CLAP に対応した DAW では、`CLAP\Simple Pitch Shift.clap` を `%LOCALAPPDATA%\Programs\Common\CLAP\` か `C:\Program Files\Common Files\CLAP\` へコピーします。
5. DAW を起動し、必要ならプラグインを再スキャンします。

インストーラーはありません。

## ダウンロードしたファイルの確かめ方

リリースのファイルには、GitHub Actions でビルドしたことを示す証明 (artifact attestation) を付けています。[GitHub CLI](https://cli.github.com/) で確かめられます。

```sh
gh attestation verify simple-pitchshift-<版>-windows-x64.zip --repo AllegroMoltoV/simple-pitchshift-audio
```

リリースは不変 (immutable release) に設定しており、公開後にファイルを差し替えられません。

## ビルド

Visual Studio 2022 と CMake 3.21 以降が必要です。依存するライブラリと SDK は、構成の時点でコミットを固定して取得します。

```sh
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release
```

CI では、単体テストに加えて、VST 3 SDK の validator、[pluginval](https://github.com/Tracktion/pluginval)、[clap-validator](https://github.com/free-audio/clap-validator) で検証します。

## ライセンス

MIT License です。[LICENSE](LICENSE) を参照してください。

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

## 更新履歴

### 1.0.0

- 最初の版です。OBS Studio 向けの版の信号処理を、VST 3 と CLAP に移植しました。
