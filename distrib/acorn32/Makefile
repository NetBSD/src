# $NetBSD: Makefile,v 1.8 2002/03/06 13:27:22 lukem Exp $

.include <bsd.own.mk>

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}
KERNELS=	INSTALL

SUBDIR=		ramdisk kernels

.include <bsd.subdir.mk>

RAMDISKOBJ!=	cd ${.CURDIR}/ramdisk && ${PRINTOBJDIR}
KERNELOBJ!=	cd ${.CURDIR}/kernels && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${RAMDISKOBJ}/ramdisk.fs ${ITARGET}/misc/ramdisk-${DISTRIBREV}.fs
	gzip -9 ${ITARGET}/misc/ramdisk-${DISTRIBREV}.fs
.for kern in ${KERNELS}
	${IINST} ${KERNELOBJ}/netbsd.${kern}.gz ${ITARGET}/kernel/
.endfor
	cd ${_SRC_TOP_}/sys/arch/acorn32/stand && \
		./export-BtNetBSD && \
		rm -f ${ITARGET}/misc/BtNetBSD.tgz && \
		gzip -9 -c BtNetBSD.tar > ${ITARGET}/misc/BtNetBSD.tgz && \
		rm -f BtNetBSD.tar && \
		mv -f BtNetBSD.zip ${ITARGET}/misc && \
	${IINST} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS ${RELEASEDIR}/ && \
	cd ${.CURDIR} && \
		rm -f ${ITARGET}/misc/gzip_ff8.bin && \
		uudecode < gzip_ff8.bin.uue && \
		mv -f gzip_ff8.bin ${ITARGET}/misc/
