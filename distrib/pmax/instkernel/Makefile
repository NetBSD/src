#	$NetBSD: Makefile,v 1.29 2002/05/02 13:26:38 simonb Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

IMAGE=		diskimage
IMAGESIZE=	2m
LISTS=		${.CURDIR}/list
IMAGEENDIAN=	le
IMAGEDEPENDS=	install.gz ${DESTDIR}/usr/mdec/boot.pmax

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} install

MDSET.install.suffixes=	ecoff
MDSET.install.post=	${ELF2ECOFF} netbsd.tmp netbsd.ecoff && \
			gzip -9f netbsd.ecoff

PRIMARYBOOT=	${DESTDIR}/usr/mdec/bootxx_ffs

${IMAGE}.gz:	${IMAGE} ${PRIMARYBOOT}
	${INSTALLBOOT} -m ${MACHINE} ${IMAGE} ${PRIMARYBOOT}
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
