# $NetBSD: Makefile,v 1.7 2002/03/06 12:39:10 lukem Exp $

.include <bsd.own.mk>

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
KERNELS=	INSTALL

SUBDIR= ramdisk kernels

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
RAMDISKOBJ != cd ${.CURDIR}/ramdisk && ${PRINTOBJDIR}
KERNELOBJ != cd ${.CURDIR}/kernels && ${PRINTOBJDIR}

release:
	${IINST} ${RAMDISKOBJ}/ramdisk-${DISTRIBREV}.fs.gz ${ITARGET}/misc/
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

