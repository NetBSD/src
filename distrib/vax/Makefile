#	$NetBSD: Makefile,v 1.5 2000/05/31 21:51:27 matt Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

SUBDIR= ramdisk ramdisk-kernel #inst tk50 rx50 rx33

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
INSTKERNOBJ != cd ramdisk-kernel; make print-objdir
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/vax/boot/boot; make print-objdir

release:
	${IINST} ${BOOTOBJ}/boot.mop ${ITARGET}/netboot/
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
.endif
