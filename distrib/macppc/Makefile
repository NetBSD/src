#	$NetBSD: Makefile,v 1.3.2.1 2000/06/23 18:01:14 matt Exp $

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
BOOTFLOPOBJ != cd floppies/bootfloppy; make print-objdir
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/macppc/stand/ofwboot; make print-objdir

release:
	${IINST} ${BOOTOBJ}/ofwboot.elf ${ITARGET}/
	${IINST} ${BOOTOBJ}/ofwboot.xcf ${ITARGET}/
	${IINST} ${BOOTFLOPOBJ}/boot.fs ${ITARGET}/
	${IINST} ${BOOTFLOPOBJ}/netbsd.ram.gz ${ITARGET}/
.endif
