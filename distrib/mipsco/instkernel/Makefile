#	$NetBSD: Makefile,v 1.7 2002/04/12 13:54:50 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
IMAGEENDIAN=	be
IMAGEDEPENDS=	${KERNELS} ${DESTDIR}/usr/mdec/boot

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} netbsd

.if defined(ECOFF_KERNEL)
netbsd.post=	${ELF2ECOFF} netbsd.tmp netbsd.ecoff && gzip -9f netbsd.ecoff

CLEANFILES+=	netbsd.ecoff netbsd.ecoff.gz
.endif # ECOFF_KERNEL

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs

${IMAGE}.gz:	${IMAGE} ${PRIMARYBOOT}
# XXX	${INSTALLBOOT} -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT}
		# XXXDISTRIB
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${PRIMARYBOOT}
	gzip -9f ${IMAGE}

CLEANFILES+=	${IMAGE}.gz


realall: ${KERNELS} ${IMAGE}.gz


DISKBINDIR=	${RELEASEDIR}/installation/diskimage
KERNBINDIR=	${RELEASEDIR}/binary/kernel

release: check_RELEASEDIR .WAIT ${KERNELS} ${IMAGE}.gz
	-mkdir -p ${DISKBINDIR} ${KERNBINDIR}
	${RELINSTALL} ${IMAGE}.gz ${DISKBINDIR}
	${RELINSTALL} netbsd.gz ${KERNBINDIR}/install.gz
.if defined(ECOFF_KERNEL)
	${RELINSTALL} netbsd.ecoff.gz ${KERNBINDIR}/install.ecoff.gz
.endif # ECOFF_KERNEL


.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
