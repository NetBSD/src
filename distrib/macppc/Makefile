#	$NetBSD: Makefile,v 1.12 2002/04/26 15:50:34 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

SUBDIR=	floppies
TARGETS+=release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/macppc/stand/ofwboot && ${PRINTOBJDIR}
ITARGET=	${RELEASEDIR}/installation

release: check_RELEASEDIR
	-mkdir -p ${ITARGET}
	${RELINSTALL} ${BOOTOBJ}/ofwboot.elf ${BOOTOBJ}/ofwboot.xcf ${ITARGET}

.include <bsd.subdir.mk>
