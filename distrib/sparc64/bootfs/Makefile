#	$NetBSD: Makefile,v 1.1 2002/05/20 21:50:10 eeh Exp $
#
# boot.fs is the image for cdrom booting.
#
# It is constructed by packaging an INSTALL kernel and ofwboot together
# into a FFS filesystem and slapping on a disklabel and bootblock.
#
# boot.fs along with an ISO image with a correct .slicemapfile in
# the root directory need to be fed to mksunbootcd to get a workable
# CDROM image.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		boot.fs
IMAGESIZE=	4m
IMAGEENDIAN=	be

LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot

RAMDISKDIR!=	cd ${.CURDIR}/../instfs && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/install.fs
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootblk

IMAGEDEPENDS=	${RAMDISKDIR}/netbsd.INSTALL.gz ${DESTDIR}/usr/mdec/ofwboot \
		${PRIMARYBOOT}
IMAGEPOSTBUILD=	${INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT} /boot

IMAGE_RELEASEDIR=	installation/misc

.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
