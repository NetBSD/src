#! /bin/sh

# Startup script to create the device-mapper control device 
# on non-devfs systems.
# Non-zero exit status indicates failure.

# These must correspond to the definitions in device-mapper.h and dm.h
DM_DIR="mapper"
DM_NAME="device-mapper"

set -e

DIR="/dev/$DM_DIR"
CONTROL="$DIR/control"

# Check for devfs, procfs
if test -e /dev/.devfsd ; then
	echo "devfs detected: devmap_mknod.sh script not required."
	exit
fi

if test ! -e /proc/devices ; then
	echo "procfs not found: please create $CONTROL manually."
	exit 1
fi

# Get major, minor, and mknod
MAJOR=$(sed -n 's/^ *\([0-9]\+\) \+misc$/\1/p' /proc/devices)
MINOR=$(sed -n "s/^ *\([0-9]\+\) \+$DM_NAME\$/\1/p" /proc/misc)

if test -z "$MAJOR" -o -z "$MINOR" ; then
	echo "$DM_NAME kernel module not loaded: can't create $CONTROL."
	exit 1
fi

mkdir -p --mode=755 $DIR
test -e $CONTROL && rm -f $CONTROL

echo "Creating $CONTROL character device with major:$MAJOR minor:$MINOR."
mknod --mode=600 $CONTROL c $MAJOR $MINOR

