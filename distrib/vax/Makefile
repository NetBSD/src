#	$NetBSD: Makefile,v 1.11 2002/03/29 06:46:34 matt Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR= ramdisk ramdisk-kernel floppy #inst tk50 rx50 rx33

.include <bsd.subdir.mk>

INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel && ${PRINTOBJDIR}
FLOPPYOBJ != cd ${.CURDIR}/floppy && ${PRINTOBJDIR}

release: check_RELEASEDIR
.if ${OBJECT_FMT} != "ELF"
	${IINST} ${DESTDIR}/usr/mdec/boot.mop ${ITARGET}/netboot/
.endif
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
	${IINST} ${FLOPPYOBJ}/boot.fs ${ITARGET}/bootfs/
