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
  https://github.com/Tracktion/pluginval/releases/download/v1.0.4/pluginval_Windows.zip \
  c08e61ce3b96db41636f8ec7e76f4c7e2c13ebdac7fa1b5a1f52b4f32ec715ab

fetch clap-validator \
  https://github.com/free-audio/clap-validator/releases/download/0.4.1/clap-validator-0.4.1-127-g152b982-windows.zip \
  d935c3af0a45c3911ea2e900f4aa5d6709dac82bb485f0c4ce28648ab2cd0c10
