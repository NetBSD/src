#	$NetBSD: Makefile,v 1.12 2002/03/31 17:27:43 matt Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR= ramdisk .WAIT ramdisk-kernel .WAIT floppy

.include <bsd.subdir.mk>

INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel && ${PRINTOBJDIR}
FLOPPYOBJ != cd ${.CURDIR}/floppy && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${DESTDIR}/usr/mdec/boot ${ITARGET}/netboot/
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
	${IINST} ${FLOPPYOBJ}/boot.fs ${ITARGET}/bootfs/
