#	$NetBSD: Makefile,v 1.10 2002/03/31 06:44:12 lukem Exp $

.include <bsd.own.mk>		# Use /etc/mk.conf.

ITARGET=	${RELEASEDIR}/installation
IINST=		${INSTALL} ${RENAME} ${PRESERVE} ${COPY} -m ${NONBINMODE}

SUBDIR=	floppies

.include <bsd.subdir.mk>

BOOTFLOPOBJ != cd ${.CURDIR}/floppies/bootfloppy && ${PRINTOBJDIR}
BOOTOBJ != cd ${.CURDIR}/../../sys/arch/macppc/stand/ofwboot && ${PRINTOBJDIR}

release: check_RELEASEDIR
	${IINST} ${BOOTOBJ}/ofwboot.elf ${ITARGET}/
	${IINST} ${BOOTOBJ}/ofwboot.xcf ${ITARGET}/
	${IINST} ${BOOTFLOPOBJ}/boot.fs ${ITARGET}/
	cd ${.CURDIR}/floppies/md-kernel && ${MAKE} release
