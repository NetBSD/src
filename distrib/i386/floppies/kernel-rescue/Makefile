#	$NetBSD: Makefile,v 1.14 2002/04/12 23:54:28 tv Exp $

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

release:	# do nothing

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
