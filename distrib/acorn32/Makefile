# $NetBSD: Makefile,v 1.5 2002/02/16 01:53:52 reinoud Exp $

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
		rm -f ${ITARGET}/misc/BtNetBSD.tgz && \
		gzip -9 -c BtNetBSD.tar > ${ITARGET}/misc/BtNetBSD.tgz && \
		rm -f BtNetBSD.tar && \
		mv -f BtNetBSD.zip ${ITARGET}/misc && \
	${IINST} ${.CURDIR}/../notes/acorn32/prep.RISCOS ${RELEASEDIR}/ && \
	cd ${.CURDIR} && \
		rm -f ${ITARGET}/misc/gzip_ff8.bin && \
		uudecode < gzip_ff8.bin.uue && \
		mv -f gzip_ff8.bin ${ITARGET}/misc/
.endif

