# $NetBSD: Makefile,v 1.9 2002/03/07 02:24:25 lukem Exp $

.include <bsd.own.mk>

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
KERNELS=	A7INST CATSINST RPCINST SHARKINST

SUBDIR= ramdisk kernels

.include <bsd.subdir.mk>

RAMDISKOBJ!=	cd ${.CURDIR}/ramdisk && ${PRINTOBJDIR}
KERNELOBJ!=	cd ${.CURDIR}/kernels && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${RAMDISKOBJ}/ramdisk-${DISTRIBREV}.fs.gz ${ITARGET}/misc/
.for kern in ${KERNELS}
	${IINST} ${KERNELOBJ}/netbsd.${kern}.gz ${ITARGET}/kernel/
.endfor
