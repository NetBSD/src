#	$NetBSD: Makefile,v 1.5 2000/08/30 23:51:50 jhawk Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY}	\
		-o ${BINOWN} -g ${BINGRP} -m ${NONBINMODE}

SUBDIR=	floppies

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
BOOTFLOPOBJ != cd floppies/bootfloppy; ${MAKE} print-objdir
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/macppc/stand/ofwboot; ${MAKE} print-objdir

release:
	${IINST} ${BOOTOBJ}/ofwboot.elf ${ITARGET}/
	${IINST} ${BOOTOBJ}/ofwboot.xcf ${ITARGET}/
	${IINST} ${BOOTFLOPOBJ}/boot.fs ${ITARGET}/
	${IINST} ${BOOTFLOPOBJ}/netbsd.ram.gz ${ITARGET}/
.endif
