#	$NetBSD: Makefile,v 1.9 2002/02/09 13:56:50 lukem Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR=	floppies

.include <bsd.subdir.mk>

.ifndef RELEASEDIR
release:
	@echo setenv RELEASEDIR first
	@false
.else
BOOTFLOPOBJ != cd ${.CURDIR}/floppies/bootfloppy && ${PRINTOBJDIR}
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/macppc/stand/ofwboot && ${PRINTOBJDIR}

release:
	${IINST} ${BOOTOBJ}/ofwboot.elf ${ITARGET}/
	${IINST} ${BOOTOBJ}/ofwboot.xcf ${ITARGET}/
	${IINST} ${BOOTFLOPOBJ}/boot.fs ${ITARGET}/
	cd ${.CURDIR}/floppies/md-kernel && ${MAKE} release
.endif
