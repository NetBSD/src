#	$NetBSD: Makefile,v 1.11 2002/04/12 05:33:59 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_S	ramdisk-rescuesmall	\
		RAMDISK_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

TARGETS=	INSTALL_SMALL	${RAMDISK_S}	\
		INSTALL_TINY	${RAMDISK_T}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
