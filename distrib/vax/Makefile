#	$NetBSD: Makefile,v 1.10 2002/03/28 02:40:18 lukem Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR= ramdisk ramdisk-kernel floppy #inst tk50 rx50 rx33

.include <bsd.subdir.mk>

INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel && ${PRINTOBJDIR}
FLOPPYOBJ != cd ${.CURDIR}/floppy && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${DESTDIR}/usr/mdec/boot.mop ${ITARGET}/netboot/
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
	${IINST} ${FLOPPYOBJ}/boot.fs ${ITARGET}/bootfs/
