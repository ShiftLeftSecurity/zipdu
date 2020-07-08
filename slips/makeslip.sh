#!/usr/bin/env bash

set -o errexit
set -o pipefail
set -o nounset

# Script starts here

# This script creates a zip specifically designed to overwrite the `execution.sh` file in a ZipDu setup.

readonly TMP_DIR_LEVEL_ONE=$(mktemp -d --tmpdir=.)
readonly TMP_DIR_LEVEL_TWO=$(mktemp -d --tmpdir="${TMP_DIR_LEVEL_ONE}")
cleanup() {
  rm -rf "${TMP_DIR_LEVEL_ONE}"
}
trap cleanup EXIT

pushd "${TMP_DIR_LEVEL_TWO}"
zip ../../slipwell.zip ../../execution.sh
popd
