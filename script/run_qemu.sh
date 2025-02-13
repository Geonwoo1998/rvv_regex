#!/bin/bash

set -e

EXEC="./rvv_test"

QEMU="qemu-riscv64"

if [ ! -f $EXEC ]; then
    echo "Error: $EXEC not found"
    exit 1
fi

echo "Running $EXEC on $QEMU"
$QEMU $EXEC