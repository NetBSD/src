#	$NetBSD: Makefile,v 1.11 2002/05/02 18:02:42 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
IMAGEENDIAN=	be
PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs
IMAGEDEPENDS=	install.gz ${DESTDIR}/usr/mdec/boot ${PRIMARYBOOT}
IMAGEPOSTBUILD=	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${PRIMARYBOOT}	# XXXDISTRIB

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} install

MDSET_RELEASEDIR=	binary/kernel
IMAGE_RELEASEDIR=	installation/diskimage

.if defined(ECOFF_KERNEL)
MDSET.install.suffixes=	ecoff
MDSET.install.post=	${ELF2ECOFF} netbsd.tmp netbsd.ecoff && \
			gzip -9f netbsd.ecoff
.endif # ECOFF_KERNEL

.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
