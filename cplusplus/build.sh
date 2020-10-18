#!/bin/bash

set -e
set -x

rm -rf build
mkdir build
pushd build

conan profile update settings.compiler.libcxx=libstdc++11 default
conan install ..
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
