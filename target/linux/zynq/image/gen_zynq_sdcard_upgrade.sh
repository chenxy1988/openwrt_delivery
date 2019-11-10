#!/usr/bin/env bash

set -ex
[ $# -eq 4 ] || {
    echo "SYNTAX: $0 <file> <dtb> <image> <rootfs>"
    exit 1
}

OUTPUT="$1"
DTB="$2"
IMAGE="$3"
ROOTFS=$4
HEADER="UPGRADE"
IMGTYPE="GZIP"
IMGPOS="END5"


rm -fr "$OUTPUT"
echo -n "$HEADER" >imghdr
echo -n "$IMGTYPE" >imgtype
echo -n "$IMGPOS" >cksumend

cp "$IMAGE" ./uimage
cp "$DTB" ./devicetree.dtb
cp "$ROOTFS" ./rootfs.bin

md5sum ./uimage >uimage.md5
md5sum ./devicetree.dtb >devicetree.dtb.md5
md5sum ./rootfs.bin > rootfs.bin.md5
tar czvf "$OUTPUT" uimage devicetree.dtb rootfs.bin devicetree.dtb.md5 uimage.md5 rootfs.bin.md5

rm -fr imghdr
rm -fr cksumend
rm -fr uimage.md5
rm -fr devicetree.dtb.md5
rm -fr image.tgz.md5
rm -fr imgtype
rm -fr image.tgz
rm -fr uimage
rm -fr devicetree.dtb
rm -fr rootfs.bin.md5
rm -fr rootfs.bin
