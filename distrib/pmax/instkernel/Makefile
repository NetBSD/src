#	$NetBSD: Makefile,v 1.25 2002/04/12 13:52:12 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
IMAGEENDIAN=	le
IMAGEDEPENDS=	netbsd.gz ${DESTDIR}/usr/mdec/boot

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} netbsd

netbsd.post=	${ELF2ECOFF} netbsd.tmp netbsd.ecoff && gzip -9f netbsd.ecoff

CLEANFILES+=	netbsd.ecoff netbsd.ecoff.gz

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs

${IMAGE}.gz:	${IMAGE} ${PRIMARYBOOT}
	${INSTALLBOOT} -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT}
	gzip -9f ${IMAGE}

CLEANFILES+=	${IMAGE}.gz


realall: ${KERNELS} ${IMAGE}.gz


DISKBINDIR=	${RELEASEDIR}/installation/diskimage
KERNBINDIR=	${RELEASEDIR}/binary/kernel

release: check_RELEASEDIR .WAIT ${KERNELS} ${IMAGE}.gz
	-mkdir -p ${DISKBINDIR} ${KERNBINDIR}
	${RELINSTALL} ${IMAGE}.gz ${DISKBINDIR}
	${RELINSTALL} netbsd.gz ${KERNBINDIR}/install.gz
	${RELINSTALL} netbsd.ecoff.gz ${KERNBINDIR}/install.ecoff.gz


.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
