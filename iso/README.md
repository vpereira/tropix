# ISO Build (Source-Only)

This directory contains the recipe to assemble a bootable Tropix ISO without
checking in binaries. The ISO is built from:

- `boot`    (boot2)
- `boot.cd` (boot1 for CD)
- `boot.gz` (compressed RAMD root filesystem image)
- `tropix`  (kernel)
- optional: `install.txt`

`winsys/` and `fdimage.exe` are not included.

## 1) Build boot loaders and kernel (inside Tropix)

Set `SRCROOT` to the Tropix source tree inside your Tropix box.

```sh
cd iso
make SRCROOT=/src/sys/tropix/pc kernel
```

## 2) Build boot.gz (inside Tropix)

You need a populated root filesystem tree (example: `/rootfs`).

```sh
cd iso
make ROOTFS=/rootfs rootfs
```

This creates `iso_root/boot.gz`.

## 3) Stage ISO root (inside Tropix)

```sh
cd iso
make stage
```

This creates `iso_root/` and copies `boot`, `boot.cd`, `tropix`, and `install.txt`.

## 4) Create ISO (host system with mkisofs/xorriso)

```sh
cd iso
make iso
```

If you prefer, run the script directly:

```sh
./build-iso.sh ./iso_root ../tropix.iso TROPIXBOOT
```

## Notes

- `boot.gz` is a 32 MB T1 image (RAMD) compressed with gzip.
- `mkisofs` (or `xorriso -as mkisofs`) is required on the host.
