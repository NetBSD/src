#	$NetBSD: Makefile,v 1.2 2000/06/08 17:39:44 matt Exp $

#	$NetBSD: Makefile,v 1.2 2000/06/08 17:39:44 matt Exp $

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
INSTKERNOBJ != cd floppies/bootfloppy; make print-objdir
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/macppc/stand/ofwboot; make print-objdir

release:
	${IINST} ${BOOTOBJ}/ofwboot.elf ${ITARGET}/
	${IINST} ${INSTKERNOBJ}/netbsd.ram.gz ${ITARGET}/netbsd.ram.gz
.endif
