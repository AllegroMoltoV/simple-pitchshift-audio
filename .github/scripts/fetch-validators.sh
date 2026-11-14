#!/usr/bin/env bash
# Downloads the validators at fixed versions and checks their SHA-256 before anything runs them.
set -euo pipefail

dest="$1"
mkdir -p "${dest}"

fetch() {
  local name="$1" url="$2" sha256="$3"
  local zip="${dest}/${name}.zip"
  curl -fsSL -o "${zip}" "${url}"
  echo "${sha256}  ${zip}" | sha256sum --check --strict
  mkdir -p "${dest}/${name}"
  unzip -q "${zip}" -d "${dest}/${name}"
}

fetch pluginval \
  https://github.com/Tracktion/pluginval/releases/download/v2.0.0-beta3/pluginval_Windows.zip \
  da158e918b60430c6a5223cd99cc2e746bda604e583258c400898caf4e95c523

fetch clap-validator \
  https://github.com/free-audio/clap-validator/releases/download/0.4.1/clap-validator-0.4.1-127-g152b982-windows.zip \
  d935c3af0a45c3911ea2e900f4aa5d6709dac82bb485f0c4ce28648ab2cd0c10
