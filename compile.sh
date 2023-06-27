#!/usr/bin/env bash

if [[ "x$CC" == "x" ]]; then
    CC=cc
fi

if [[ "x$CHIBICC_DIR" == "x" ]]; then
    CHIBICC_DIR=../chibicc
fi

set -e

# preprocess
"$CC" -I"$CHIBICC_DIR" -P -E -x c "./oneko-uxn/oneko.c" -o tmp.c
# compile
"$CHIBICC_DIR"/chibicc -O tmp.c
