# $NetBSD: Makefile,v 1.2 2000/11/12 00:51:36 matt Exp $

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}
KERNELS=	A7INST CATSINST RPCINST SHARKINST

SUBDIR= ramdisk kernels

REV!=sh ${BSDSRCDIR}/sys/conf/osrelease.sh -s

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
RAMDISKOBJ != cd ${.CURDIR}/ramdisk; ${MAKE} print-objdir
KERNELOBJ != cd ${.CURDIR}/kernels; ${MAKE} print-objdir

release:
	${IINST} ${RAMDISKOBJ}/ramdisk-${REV}.fs.gz ${ITARGET}/misc/
.for kern in ${KERNELS}
	${IINST} ${KERNELOBJ}/netbsd.${kern}.gz ${ITARGET}/kernel/
.endfor
.endif
