#	$NetBSD: Makefile,v 1.7 2000/09/12 16:06:55 matt Exp $

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
INSTKERNOBJ != cd ${.CURDIR}/ramdisk-kernel; ${MAKE} print-objdir
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/vax/boot/boot; ${MAKE} print-objdir

release:
	${IINST} ${BOOTOBJ}/boot.mop ${ITARGET}/netboot/
	${IINST} ${INSTKERNOBJ}/netbsd.INSTALL.gz ${ITARGET}/netboot/netbsd.ram.gz
.endif
