#	$NetBSD: Makefile,v 1.31 2002/05/03 14:20:55 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
MTREECONF=	${DISTRIBDIR}/common/mtree.dot
IMAGEENDIAN=	le
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs
IMAGEDEPENDS=	install.gz ${DESTDIR}/usr/mdec/boot.pmax ${PRIMARYBOOT}
IMAGEPOSTBUILD=	${INSTALLBOOT} -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT}

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} install

MDSET_RELEASEDIR=	binary/kernel
IMAGE_RELEASEDIR=	installation/diskimage

MDSET.install.suffixes=	ecoff
MDSET.install.post=	${ELF2ECOFF} netbsd.tmp netbsd.ecoff && \
			gzip -9f netbsd.ecoff

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
