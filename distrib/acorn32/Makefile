# $NetBSD: Makefile,v 1.11 2002/04/11 06:40:48 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

ITARGET=	${RELEASEDIR}/installation
KERNELS=	INSTALL

SUBDIR=		ramdisk .WAIT kernels

.include <bsd.subdir.mk>

RAMDISKOBJ!=	cd ${.CURDIR}/ramdisk && ${PRINTOBJDIR}
KERNELOBJ!=	cd ${.CURDIR}/kernels && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${RELINSTALL} ${RAMDISKOBJ}/ramdisk.fs \
	    ${ITARGET}/misc/ramdisk-${DISTRIBREV}.fs
	gzip -9 ${ITARGET}/misc/ramdisk-${DISTRIBREV}.fs
.for kern in ${KERNELS}
	${RELINSTALL} ${KERNELOBJ}/netbsd.${kern}.gz ${ITARGET}/kernel/
.endfor
	cd ${_SRC_TOP_}/sys/arch/acorn32/stand && \
		./export-BtNetBSD && \
		rm -f ${ITARGET}/misc/BtNetBSD.tgz && \
		gzip -9 -c BtNetBSD.tar > ${ITARGET}/misc/BtNetBSD.tgz && \
		rm -f BtNetBSD.tar && \
		mv -f BtNetBSD.zip ${ITARGET}/misc && \
	${RELINSTALL} ${DISTRIBDIR}/notes/acorn32/prep.RISCOS ${RELEASEDIR}/ && \
	cd ${.CURDIR} && \
		rm -f ${ITARGET}/misc/gzip_ff8.bin && \
		uudecode < gzip_ff8.bin.uue && \
		mv -f gzip_ff8.bin ${ITARGET}/misc/
