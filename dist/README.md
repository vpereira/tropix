# dist — Pre-built Tropix Binary Artifacts

This directory holds the compiled binary artifacts used to assemble
the bootable `tropix.iso`. They are built **inside a running Tropix VM**
and exported here so that GitHub Actions can assemble new ISO releases
without needing the Tropix toolchain on the CI host.

## Files

| File | Size | Description | Source (inside Tropix) |
|------|------|-------------|------------------------|
| `boot.cd` | ~432 B | El Torito CD boot sector (boot1) | `kernel/boot/boot1/cd.boot1` |
| `boot` | ~138 KB | Stage-2 bootloader (boot2) | `kernel/boot/boot2/boot` |
| `tropix` | ~563 KB | Kernel binary | `kernel/kernel/tropix` |
| `boot.gz` | ~9.4 MB | Compressed 32 MB T1 root filesystem | assembled by `iso/build-rootfs.sh` |

## How to update

After making source changes and building inside your Tropix VM, export
updated artifacts with the helper script:

```sh
# Tropix VM must be running with QEMU user networking
# (port 21 forwarded, FTP configured, network up)
./scripts/export-from-tropix.sh <tropix-vm-ip>
```

Or manually via FTP:

```sh
ftp -n <tropix-vm-ip> <<EOF
user root tropix
binary
get /tropix         dist/tropix
get /etc/boot/boot  dist/boot
get /etc/boot/cd.boot1 dist/boot.cd
get /tmp/boot.gz    dist/boot.gz
bye
EOF
```

Then commit the changed artifacts and push a new tag to trigger a release.

## Release workflow

1. Build and test inside your Tropix VM
2. Export updated `dist/` artifacts (see above)
3. Commit: `git add dist/ && git commit -m "Update built artifacts for vX.Y.Z"`
4. Tag: `git tag vX.Y.Z && git push && git push --tags`
5. GitHub Actions assembles the ISO and publishes a GitHub Release automatically
