#	$NetBSD: Makefile,v 1.11 2002/04/12 02:19:30 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

.include <bsd.kernobj.mk>

SUBDIR=	floppies
TARGETS+=release

BOOTOBJ!=	cd ${KERNSRCDIR}/arch/macppc/stand/ofwboot && ${PRINTOBJDIR}
ITARGET=	${RELEASEDIR}/installation

release: check_RELEASEDIR
	-mkdir -p ${ITARGET}
	${RELINSTALL} ${BOOTOBJ}/ofwboot.elf ${BOOTOBJ}/ofwboot.xcf ${ITARGET}

.include <bsd.subdir.mk>
