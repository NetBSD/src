# $NetBSD: Makefile,v 1.4 2002/02/11 23:07:46 reinoud Exp $

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
KERNELS=	INSTALL

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
	cd ${.CURDIR}/../../sys/arch/acorn32/stand && \
		./export-BtNetBSD && \
		mv -f BtNetBSD.tar ${ITARGET}/misc && \
		rm -rf ${ITARGET}/misc/BtNetBSD.tar.gz
		gzip ${ITARGET}/misc/BtNetBSD.tar && \
		rm -rf BtNetBSD.tar 
.endif
