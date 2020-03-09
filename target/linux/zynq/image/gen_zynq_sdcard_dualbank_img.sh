#!/usr/bin/env bash

set -ex
[ $# -eq 6 ] || {
    echo "SYNTAX: $0 <file> <bootfs image> <bank1> <bank2> <rootfs image> <bootfs size> <rootfs size>"
    exit 1
}

OUTPUT="$1"
BOOTFS="$2"
BANK2="$3"
ROOTFS="$4"
BOOTFSSIZE="$5"
ROOTFSSIZE="$6"
FREESPACE="5120"

head=4
sect=63

set $(ptgen -o $OUTPUT -h $head -s $sect -l 1024 -t c -p ${BOOTFSSIZE}M -t 83 -p ${ROOTFSSIZE}M -t 83 -p ${ROOTFSSIZE}M -t 83 -p ${FREESPACE}M)

BOOTOFFSET="$(($1 / 512))"
BOOTSIZE="$(($2 / 512))"
BANKA_OFFSET="$(($BOOTSIZE + $BOOTOFFSET))"
BANKB_OFFSET="$(($BANKA_OFFSET + $BOOTSIZE + $BOOTOFFSET))"
ROOTFSOFFSET="$(($BANKB_OFFSET + $BOOTSIZE + $BOOTOFFSET))"
#ROOTFSOFFSET="$(($3 / 512))"
ROOTFSSIZE="$(($4 / 512))"

###FLASH LAYOUT:
###---Header: 2048
###---Payload : Bootsize
###---Header 2048
###---Payload: Bank size
### .......
###---Header 2048
###---Palload: Overlay
###
###
dd bs=512 if="$BOOTFS" of="$OUTPUT" seek="$BOOTOFFSET" conv=notrunc
dd bs=512 if="$ROOTFS" of="$OUTPUT" seek=36864 conv=notrunc
#dd bs=512 if="$ROOTFS" of="$OUTPUT" seek=102400 conv=notrunc
