#	$NetBSD: Makefile,v 1.6 2006/06/08 08:40:39 he Exp $
#
# boot.fs is the image for cdrom booting.
#
# It is constructed by packaging an INSTALL kernel and ofwboot together
# into a FFS filesystem and slapping on a disklabel and bootblock.
#
# boot.fs along with an ISO image need to be fed to distrib/common/sunbootcd.sh
# to get a workable CDROM image.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		boot.fs
IMAGESIZE=	5m
IMAGEENDIAN=	be

LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot

RAMDISKDIR!=	cd ${.CURDIR}/../instfs && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/install.fs
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootblk

PARSELISTENV=	RAMDISKDIR=${RAMDISKDIR:Q}
IMAGEDEPENDS=	${RAMDISKDIR}/netbsd-INSTALL.gz ${DESTDIR}/usr/mdec/ofwboot \
		${PRIMARYBOOT}
IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT} /boot

IMAGE_RELEASEDIR=	installation/misc

.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
