#!/bin/bash

set -e

EXEC="./rvv_test"

SCRIPT="/workspace/rvv/gem5_script.py"

GEM5="/workspace/gem5_origin/gem5/build/RISCV/gem5.opt"

if [ ! -f $EXEC ]; then
    echo "Error: $EXEC not found"
    exit 1
fi

if [ ! -f $SCRIPT ]; then
    echo "Error: $SCRIPT not found"
    exit 1
fi

echo "Running $EXEC on $GEM5"
$GEM5 -d m5out/ $SCRIPT -c $EXEC