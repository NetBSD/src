#	$NetBSD: Makefile,v 1.10 2002/04/11 15:34:55 lukem Exp $

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
