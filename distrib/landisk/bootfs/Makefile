#	$NetBSD: Makefile,v 1.1.6.2 2008/06/02 13:21:14 mjf Exp $
#
# boot.fs is the image for wd booting.
#
# It is constructed by packaging an INSTALL kernel into a FFS
# filesystem and slapping on a disklabel and bootblock.
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		boot.fs
IMAGESIZE=	10m
IMAGEENDIAN=	le

LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot

RAMDISKDIR!=	cd ${.CURDIR}/../instkernel && ${PRINTOBJDIR}
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffsv1
SECONDARYBOOT=	${DESTDIR}/usr/mdec/boot

PARSELISTENV=	RAMDISKDIR=${RAMDISKDIR:Q}
IMAGEDEPENDS=	${RAMDISKDIR}/netbsd-INSTALL.gz  \
		${PRIMARYBOOT} ${SECONDARYBOOT}
IMAGEPOSTBUILD=	${TOOL_INSTALLBOOT} -v -m ${MACHINE} ${IMAGE} \
		${PRIMARYBOOT} ${SECONDARYBOOT}

IMAGE_RELEASEDIR=	installation/misc

.include "${DISTRIBDIR}/common/Makefile.image"

.include <bsd.prog.mk>
