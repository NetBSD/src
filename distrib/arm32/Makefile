# $NetBSD: Makefile,v 1.7 2002/03/05 21:23:29 thorpej Exp $

.include <bsd.own.mk>

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
KERNELS=	A7INST CATSINST RPCINST SHARKINST

SUBDIR= ramdisk kernels

REV!=sh ${.CURDIR}/../../sys/conf/osrelease.sh -s

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
RAMDISKOBJ != cd ${.CURDIR}/ramdisk && ${PRINTOBJDIR}
KERNELOBJ != cd ${.CURDIR}/kernels && ${PRINTOBJDIR}

release:
	${IINST} ${RAMDISKOBJ}/ramdisk-${REV}.fs.gz ${ITARGET}/misc/
.for kern in ${KERNELS}
	${IINST} ${KERNELOBJ}/netbsd.${kern}.gz ${ITARGET}/kernel/
.endfor
.endif
