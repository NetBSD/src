#	$NetBSD: Makefile,v 1.16 2002/04/11 12:17:14 lukem Exp $

# This Makefile builds a boot image on a 2.88M-sized image.
#
# Since NetBSD currently can't write 2.88M diskettes, it's only
# use is as a boot image for El Torito bootable CD-ROM images.
#
# To prepare a CD, do the following steps:
# 1) create a release(7) tree called NetBSD-1.3I
# 2) create NetBSD-1.3I/boot.i386/ and place boot.fs in that directory
# 3) do ``mkisofs -A "NetBSD ${version}" -b boot.i386/boot.fs -f \
#	-o i386-cd.img -r -T NetBSD-1.3I''
#    (you may omit '-f' if you're not using symlinks in the release tree)
# This should place the image in i386-cd.img, which can hopefully
# be used to burn a CD.

IMAGE=		boot-big.fs
KERNELDIR=	kernel-ramdisk
KERNEL=		netbsd.INSTALL.gz
DISKSIZE=	5760
METAFILE!=	printf "USTAR.volsize.%o" ${DISKSIZE}
BLOCK8K=	359
PAD=		yes

.include "${.CURDIR}/../common/Makefile.bootfloppy"
