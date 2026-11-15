Simple Pitch Shift / シンプルピッチシフト
========================================

Changes voice pitch and formant in real time. Stereo audio effect in VST® 3 and CLAP formats for Windows x64 and Linux x86_64.
声の高さと声質をリアルタイムに変えるステレオの音声エフェクトです。Windows x64 と Linux x86_64 向けの VST® 3 と CLAP の形式で配布します。

Source code and verification / ソースコードと検証:
  https://github.com/AllegroMoltoV/simple-pitchshift-audio

Install / 導入
--------------

1. Close your DAW. / DAW を終了します。
2. Windows: copy the folder "VST3\Simple Pitch Shift.vst3" to one of:
   Windows では、「VST3\Simple Pitch Shift.vst3」フォルダーを次のどちらかへコピーします。
     %LOCALAPPDATA%\Programs\Common\VST3\   (only you / 自分だけ)
     C:\Program Files\Common Files\VST3\     (all users, needs administrator / 全ユーザー。管理者権限が必要)
3. Windows, for CLAP hosts: copy "CLAP\Simple Pitch Shift.clap" to one of:
   Windows の CLAP に対応した DAW では、「CLAP\Simple Pitch Shift.clap」を次のどちらかへコピーします。
     %LOCALAPPDATA%\Programs\Common\CLAP\
     C:\Program Files\Common Files\CLAP\
4. Linux: copy the folder "VST3/Simple Pitch Shift.vst3" to ~/.vst3/ and "CLAP/Simple Pitch Shift.clap" to ~/.clap/.
   Linux では、「VST3/Simple Pitch Shift.vst3」フォルダーを ~/.vst3/ へ、「CLAP/Simple Pitch Shift.clap」を ~/.clap/ へコピーします。
5. Start your DAW and rescan plug-ins if needed. / DAW を起動し、必要ならプラグインを再スキャンします。

No installer and no network access. The plug-in does not send any audio or data anywhere.
インストーラーはありません。プラグインは通信しません。音声やデータを外部へ送りません。

Parameters / パラメーター
------------------------

Pitch    -12 to +12 semitones / 半音
Formant  -12 to +12 semitones / 半音
Gain     -30 to +30 dB
Window   20, 30 or 40 ms. Shorter is more responsive, longer is smoother.
         短いほど追従が速く、長いほど滑らかです。

The latency is constant for a given sample rate, so the DAW's delay compensation stays aligned.
レイテンシーはサンプルレートごとに一定で、DAW の遅延補正がずれません。

Licenses / ライセンス
--------------------

Simple Pitch Shift is released under the MIT License (LICENSE.txt).
Third-party licenses are in THIRD_PARTY_LICENSES.

VST is a registered trademark of Steinberg Media Technologies GmbH.
