#	$NetBSD: Makefile,v 1.9 2002/04/14 06:18:13 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
IMAGEENDIAN=	be
IMAGEDEPENDS=	install.gz ${DESTDIR}/usr/mdec/boot

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} install

.if defined(ECOFF_KERNEL)
MDSET.install.suffixes=	ecoff
MDSET.install.post=	${ELF2ECOFF} netbsd.tmp netbsd.ecoff && \
			gzip -9f netbsd.ecoff
.endif # ECOFF_KERNEL

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs

${IMAGE}.gz:	${IMAGE} ${PRIMARYBOOT}
# XXX	${INSTALLBOOT} -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT}
		# XXXDISTRIB
	${DESTDIR}/usr/mdec/installboot ${IMAGE} ${PRIMARYBOOT}
	gzip -9f ${IMAGE}

CLEANFILES+=	${IMAGE}.gz


realall: ${KERNELDEPS} ${IMAGE}.gz


DISKBINDIR=	${RELEASEDIR}/installation/diskimage
KERNBINDIR=	${RELEASEDIR}/binary/kernel

release: check_RELEASEDIR .WAIT ${IMAGE}.gz ${KERNELDEPS}
	-mkdir -p ${DISKBINDIR} ${KERNBINDIR}
	${RELINSTALL} ${IMAGE}.gz ${DISKBINDIR}
	${RELINSTALL} ${KERNELS} ${KERNBINDIR}


.include "${DISTRIBDIR}/common/Makefile.image"
.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
