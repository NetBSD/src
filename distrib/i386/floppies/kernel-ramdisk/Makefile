#	$NetBSD: Makefile,v 1.17 2002/04/11 15:34:55 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big	\
		RAMDISK_S	ramdisk-small	\
		RAMDISK_T	ramdisk-tiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

TARGETS=	INSTALL		${RAMDISK_B}	\
		INSTALL_LAPTOP	${RAMDISK_B}	\
		INSTALL_SMALL	${RAMDISK_S}	\
		INSTALL_TINY	${RAMDISK_T}	\
		INSTALL_PS2	${RAMDISK_S}

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"
