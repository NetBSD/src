#	$NetBSD: Makefile,v 1.3 2000/06/12 23:02:45 matt Exp $

#	$NetBSD: Makefile,v 1.3 2000/06/12 23:02:45 matt Exp $

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
	${IINST} ${BOOTOBJ}/ofwboot.xcf ${ITARGET}/
	${IINST} ${INSTKERNOBJ}/netbsd.ram.gz ${ITARGET}/netbsd.ram.gz
.endif
