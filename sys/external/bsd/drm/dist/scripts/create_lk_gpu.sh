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

DRMDIR=$1/drivers/gpu/drm/
HDRDIR=$1/include/drm/
KERNEL_VERS=$2
echo "Copying kernel independent files"
mkdir -p $DRMDIR/.tmp
mkdir -p $HDRDIR/.tmp

( cd linux-core/ ; make drm_pciids.h )
cp shared-core/*.[ch] $DRMDIR/.tmp
cp linux-core/*.[ch] $DRMDIR/.tmp
cp linux-core/Makefile.kernel $DRMDIR/.tmp/Makefile

echo "Copying 2.6 Kernel files"
cp linux-core/Kconfig $DRMDIR/.tmp

./scripts/drm-scripts-gentree.pl $KERNEL_VERS $DRMDIR/.tmp $DRMDIR
mv $DRMDIR/drm*.h $HDRDIR
mv $DRMDIR/*_drm.h $HDRDIR

cd $DRMDIR
rm -rf .tmp
rm sis_ds.[ch]
rm amd*.[ch]
rm radeon_ms*.[ch]

for i in radeon mach64 r128 mga i915 i810 savage sis xgi nouveau tdfx ffb imagine
do
mkdir ./$i
mv $i*.[ch] $i/
done

mv r300*.[ch] radeon/
mv r600*.[ch] radeon/
mv ObjectID.h radeon/
mv atom*.[ch] radeon/

mv nv*.[ch] nouveau/
mv intel*.[ch] i915/
mv dvo*.[ch] i915/

cd -
