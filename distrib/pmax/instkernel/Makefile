#	$NetBSD: Makefile,v 1.30 2002/05/02 18:02:48 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
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
