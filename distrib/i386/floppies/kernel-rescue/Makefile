#	$NetBSD: Makefile,v 1.12 2002/04/12 13:39:46 lukem Exp $

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

MDSETTARGETS=	INSTALL_SMALL	${RAMDISK_S}	-	\
		INSTALL_TINY	${RAMDISK_T}	-

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
