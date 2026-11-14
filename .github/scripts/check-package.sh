#!/usr/bin/env bash
# Fails when the release layout misses a plug-in or a license text. A published immutable release cannot be fixed.
set -euo pipefail

root="$1"
required=(
  "VST3/Simple Pitch Shift.vst3/Contents/x86_64-win/Simple Pitch Shift.vst3"
  "CLAP/Simple Pitch Shift.clap"
  "LICENSE.txt"
  "README.txt"
  "THIRD_PARTY_LICENSES/vst3sdk.txt"
  "THIRD_PARTY_LICENSES/clap.txt"
  "THIRD_PARTY_LICENSES/clap-wrapper.txt"
  "THIRD_PARTY_LICENSES/fmt.txt"
  "THIRD_PARTY_LICENSES/signalsmith-stretch.txt"
  "THIRD_PARTY_LICENSES/signalsmith-linear.txt"
)
missing=0
for path in "${required[@]}"; do
  if [ ! -s "${root}/${path}" ]; then
    echo "missing: ${path}"
    missing=1
  fi
done
# Only the files above belong in the archive: no installers or scripts.
extra=$(cd "${root}" && find . -type f | sed 's#^\./##' | grep -v -x -F "$(printf '%s\n' "${required[@]}")" || true)
if [ -n "${extra}" ]; then
  echo "unexpected files:"
  echo "${extra}"
  missing=1
fi
exit "${missing}"
