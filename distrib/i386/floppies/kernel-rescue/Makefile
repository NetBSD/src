#	$NetBSD: Makefile,v 1.16 2002/04/26 15:50:33 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_S	ramdisk-rescuesmall	\
		RAMDISK_T	ramdisk-rescuetiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

MDSETTARGETS=	INSTALL_SMALL	${RAMDISK_S}	netbsd.RESCUE_SMALL	\
		INSTALL_TINY	${RAMDISK_T}	netbsd.RESCUE_TINY

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
