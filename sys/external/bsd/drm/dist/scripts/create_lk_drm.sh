#! /bin/bash
# script to create a Linux Kernel tree from the DRM tree for diffing etc..
#
# Original author - Dave Airlie (C) 2004 - airlied@linux.ie
# kernel_version to remove below (e.g. 2.6.24)

if [ $# -lt 2 ] ;then
	echo usage: $0 output_dir kernel_version
	exit 1
fi

if [ ! -d shared-core -o ! -d linux-core ]  ;then
	echo not in DRM toplevel
	exit 1
fi

OUTDIR=$1/drivers/char/drm/
KERNEL_VERS=$2
echo "Copying kernel independent files"
mkdir -p $OUTDIR/.tmp

( cd linux-core/ ; make drm_pciids.h )
cp shared-core/*.[ch] $OUTDIR/.tmp
cp linux-core/*.[ch] $OUTDIR/.tmp
cp linux-core/Makefile.kernel $OUTDIR/.tmp/Makefile

echo "Copying 2.6 Kernel files"
cp linux-core/Kconfig $OUTDIR/.tmp

./scripts/drm-scripts-gentree.pl $KERNEL_VERS $OUTDIR/.tmp $OUTDIR
cd $OUTDIR

rm -rf .tmp
rm sis_ds.[ch]

cd -
