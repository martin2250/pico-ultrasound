#!/bin/bash

set -e

cmake --build build

echo -n "waiting for pi "
while [ ! -b "/dev/disk/by-label/RPI-RP2" ]; do
    echo -n "."
    sleep 0.1
done
echo ""

udisksctl mount --block-device /dev/disk/by-label/RPI-RP2 && cp build/*.uf2 /run/media/martin/RPI-RP2/
