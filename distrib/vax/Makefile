#	$NetBSD: Makefile,v 1.5.2.2 2001/03/20 18:35:51 he Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

SUBDIR= ramdisk ramdisk-kernel floppy #inst tk50 rx50 rx33

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel && ${PRINTOBJDIR}
FLOPPYOBJ != cd ${.CURDIR}/floppy && ${PRINTOBJDIR}

release:
	${IINST} ${DESTDIR}/usr/mdec/boot.mop ${ITARGET}/netboot/
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
	${IINST} ${FLOPPYOBJ}/boot.fs ${ITARGET}/bootfs/
.endif
