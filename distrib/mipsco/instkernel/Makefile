#	$NetBSD: Makefile,v 1.18 2019/01/01 19:09:12 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	3m
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	be
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs
IMAGEDEPENDS=	netbsd-INSTALL.gz ${DESTDIR}/usr/mdec/boot ${PRIMARYBOOT}

# XXXDISTRIB: disabled because it's not cross friendly...
# C.f, [port-mips/19349]
#
#IMAGEPOSTBUILD=	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${PRIMARYBOOT}

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} netbsd-INSTALL
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.netbsd-INSTALL=	ecoff create-ecoff
create-ecoff=			${MIPS_ELF2ECOFF} ${.TARGET:R} ${.TARGET}

IMAGE_RELEASEDIR=	installation/diskimage

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
