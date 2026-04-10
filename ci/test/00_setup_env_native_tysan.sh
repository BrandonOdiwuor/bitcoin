#!/usr/bin/env bash
#
# Copyright (c) 2026-present The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

export LC_ALL=C.UTF-8

export CONTAINER_NAME=ci_native_tysan
export CI_IMAGE_NAME_TAG="mirror.gcr.io/ubuntu:24.04"
export APT_LLVM_V="22"
export TYSAN_FLAGS="-fsanitize-ignorelist=${BASE_ROOT_DIR}/test/sanitizer_suppressions/tysan"
export PACKAGES="clang-${APT_LLVM_V} \
                 llvm-${APT_LLVM_V} \
                 llvm-${APT_LLVM_V}-dev \
                 libclang-${APT_LLVM_V}-dev \
                 libclang-rt-${APT_LLVM_V}-dev \
                 libc++-${APT_LLVM_V}-dev \
                 libc++abi-${APT_LLVM_V}-dev \
                 lld-${APT_LLVM_V} \
                 python3-zmq \
                 python3-pip"
export PIP_PACKAGES="--break-system-packages pycapnp"
export DEP_OPTS="CC=clang-${APT_LLVM_V} \
                 CXX=clang++-${APT_LLVM_V} \
                 CFLAGS='${TYSAN_FLAGS}' \
                 CXXFLAGS='${TYSAN_FLAGS} -stdlib=libc++' \
                 NM=llvm-nm-${APT_LLVM_V} \
                 AR=llvm-ar-${APT_LLVM_V} \
                 RANLIB=llvm-ranlib-${APT_LLVM_V} \
                 STRIP=llvm-strip-${APT_LLVM_V} \
                 NO_QT=1"
export GOAL="install"
export BITCOIN_CONFIG="\
 --preset=dev-mode \
 -DBUILD_GUI=OFF \
 -DCMAKE_C_FLAGS_DEBUG='' \
 -DCMAKE_CXX_FLAGS_DEBUG='' \
 -DSANITIZERS=type
"
export RUN_TYSAN=true
export CI_LIMIT_STACK_SIZE=1
export RUN_UNIT_TESTS=false
export RUN_FUNCTIONAL_TESTS=false
export RUN_FUZZ_TESTS=false
