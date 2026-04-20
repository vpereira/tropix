#!/bin/sh

# Gera a imagem ISO usando mkisofs ou xorriso.

ISOROOT=./iso_root
ISO_OUT=../tropix.iso
LABEL=TROPIXBOOT

if [ "x$1" != "x" ]; then
	ISOROOT=$1
fi
if [ "x$2" != "x" ]; then
	ISO_OUT=$2
fi
if [ "x$3" != "x" ]; then
	LABEL=$3
fi

if [ ! -d "$ISOROOT" ]; then
	echo "Diretorio ISO nao encontrado: $ISOROOT"
	exit 1
fi

if [ ! -f "$ISOROOT/boot.cd" ]; then
	echo "Arquivo boot.cd nao encontrado em $ISOROOT"
	exit 1
fi

if mkisofs -version >/dev/null 2>&1; then
	MKISO=mkisofs
elif xorriso -version >/dev/null 2>&1; then
	MKISO="xorriso -as mkisofs"
else
	echo "Precisa mkisofs ou xorriso"
	exit 1
fi

$MKISO -v -o "$ISO_OUT" -V "$LABEL" -J -R -b boot.cd -no-emul-boot "$ISOROOT"
