#	$NetBSD: Makefile,v 1.3 2007/12/17 13:47:29 joerg Exp $

# This Makefile builds a boot image on a 2.88M-sized image.
#
# Since NetBSD currently can't write 2.88M diskettes, its only
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

FLOPPYBASE=	boot-big
FLOPPYPAD=	1
FLOPPYKERNEL=	netbsd-INSTALL.gz

.include "${.CURDIR}/../common/Makefile.bootfloppy"
