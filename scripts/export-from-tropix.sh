#!/bin/sh
#
# export-from-tropix.sh — Export built Tropix artifacts from a running VM via FTP
#
# Usage:
#   ./scripts/export-from-tropix.sh [tropix-ip] [ftp-port]
#
# Defaults:
#   tropix-ip  = 10.0.2.15  (QEMU user networking DHCP address)
#   ftp-port   = 21
#
# The Tropix VM must be running with FTP accessible.
# With QEMU user networking + port forwarding use:
#   -netdev user,id=net0,hostfwd=tcp::2121-:21 -device rtl8139,netdev=net0
# and pass 127.0.0.1 2121 as arguments.
#
# Artifacts exported:
#   /tropix              -> dist/tropix       (kernel)
#   /etc/boot/boot       -> dist/boot         (boot2 stage-2 loader)
#   /etc/boot/cd.boot1   -> dist/boot.cd      (CD El Torito boot sector)
#   /tmp/boot.gz         -> dist/boot.gz      (compressed root filesystem)
#
# To prepare boot.gz inside Tropix before running this script:
#   cd /usr/src/iso && make ROOTFS=/ rootfs
# This produces iso_root/boot.gz; then: cp /usr/src/iso/iso_root/boot.gz /tmp/boot.gz
#

set -e

IP=${1:-10.0.2.15}
PORT=${2:-21}

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DIST="$REPO_ROOT/dist"

echo "Exporting Tropix artifacts from ftp://$IP:$PORT ..."
echo "Destination: $DIST"

export_file () {
	REMOTE=$1
	LOCAL=$2
	echo "  $REMOTE -> $LOCAL"
	curl -s --connect-timeout 10 \
		--user "root:tropix" \
		"ftp://$IP:$PORT$REMOTE" \
		-o "$LOCAL" || { echo "ERROR: failed to fetch $REMOTE"; exit 1; }
}

export_file /tropix             "$DIST/tropix"
export_file /etc/boot/boot      "$DIST/boot"
export_file /etc/boot/cd.boot1  "$DIST/boot.cd"
export_file /tmp/boot.gz        "$DIST/boot.gz"

echo ""
echo "Done. Verify sizes:"
ls -lh "$DIST/boot" "$DIST/boot.cd" "$DIST/tropix" "$DIST/boot.gz"
echo ""
echo "Next steps:"
echo "  git add dist/"
echo "  git commit -m 'Update built artifacts for vX.Y.Z'"
echo "  git tag vX.Y.Z && git push && git push --tags"
