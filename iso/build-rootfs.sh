#!/bin/sh

# Gera boot.gz a partir de um rootfs pronto.

ROOTFS=/rootfs
ISOROOT=./iso_root
TMPIMG=./boot.img
MNT=/mnt/ramd
SIZE=32M

if [ "x$1" != "x" ]; then
	ROOTFS=$1
fi
if [ "x$2" != "x" ]; then
	ISOROOT=$2
fi
if [ "x$3" != "x" ]; then
	TMPIMG=$3
fi
if [ "x$4" != "x" ]; then
	MNT=$4
fi
if [ "x$5" != "x" ]; then
	SIZE=$5
fi

if [ ! -d "$ROOTFS" ]; then
	echo "Rootfs nao encontrado: $ROOTFS"
	exit 1
fi

if [ -f "$TMPIMG" ]; then
	rm "$TMPIMG"
fi
if [ -f "$TMPIMG.gz" ]; then
	rm "$TMPIMG.gz"
fi

mkfst1 -rf "$TMPIMG" "$SIZE" TROPIXROOT ramd0 || exit 1

if [ ! -d "$MNT" ]; then
	mkdir "$MNT" || exit 1
fi
mount "$TMPIMG" "$MNT" || exit 1

cptree -s "$ROOTFS" "$MNT" || exit 1

umount "$MNT" || exit 1

gzip -9 "$TMPIMG" || exit 1

if [ ! -d "$ISOROOT" ]; then
	mkdir "$ISOROOT" || exit 1
fi
mv "$TMPIMG.gz" "$ISOROOT/boot.gz" || exit 1

echo "boot.gz criado em $ISOROOT/boot.gz"
