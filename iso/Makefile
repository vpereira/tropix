#*
#****************************************************************
#*	Makefile para a geracao de ISO				*
#****************************************************************
#*
REPO	= ..
SRCROOT	= /src/sys/tropix/pc
ISOROOT	= ./iso_root
ISO_OUT	= ../tropix.iso
LABEL	= TROPIXBOOT
ROOTFS	= /rootfs

BOOT2	= $(SRCROOT)/kernel/boot/boot2/boot
BOOTCD	= $(SRCROOT)/kernel/boot/boot1/cd.boot1
KERNEL	= $(SRCROOT)/kernel/kernel/tropix

.virtual	kernel stage rootfs iso clean

kernel:
		cd $(SRCROOT)/kernel/boot/boot1;	make cd.boot1
		cd $(SRCROOT)/kernel/boot/boot2;	make
		cd $(SRCROOT)/kernel/kernel;	make

stage:
		mkdir -f $(ISOROOT)
		cp -f $(BOOT2) $(ISOROOT)/boot
		cp -f $(BOOTCD) $(ISOROOT)/boot.cd
		cp -f $(KERNEL) $(ISOROOT)/tropix
		-cp -f $(REPO)/install.txt $(ISOROOT)/install.txt

rootfs:
		sh ./build-rootfs.sh $(ROOTFS) $(ISOROOT)

iso:
		sh ./build-iso.sh $(ISOROOT) $(ISO_OUT) $(LABEL)

clean:
		-rm -f $(ISOROOT)/boot.gz
