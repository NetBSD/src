#	$NetBSD: Makefile,v 1.13 2002/04/26 15:04:30 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

ITARGET=	${RELEASEDIR}/installation

SUBDIR= ramdisk .WAIT ramdisk-kernel .WAIT floppy

INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel && ${PRINTOBJDIR}
FLOPPYOBJ != cd ${.CURDIR}/floppy && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${RELINSTALL} ${DESTDIR}/usr/mdec/boot ${ITARGET}/netboot/
	${RELINSTALL} ${INSTKERNOBJ}/netbsd.INSTALL.gz \
	    ${ITARGET}/netboot/netbsd.ram.gz
	${RELINSTALL} ${FLOPPYOBJ}/boot.fs ${ITARGET}/bootfs/

.include <bsd.subdir.mk>
