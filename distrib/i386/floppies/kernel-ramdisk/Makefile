#	$NetBSD: Makefile,v 1.22 2002/05/08 21:44:56 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

#	create ${RAMDISK_*} variables
#
RAMDISKS=	RAMDISK_B	ramdisk-big	\
		RAMDISK_S	ramdisk-small	\
		RAMDISK_T	ramdisk-tiny

.for V F in ${RAMDISKS}
${V}DIR!=	cd ${.CURDIR}/../${F} && ${PRINTOBJDIR}
${V}=		${${V}DIR}/${F}.fs
.endfor

MDSETTARGETS=		INSTALL		${RAMDISK_B}	-	\
			INSTALL_LAPTOP	${RAMDISK_B}	-	\
			INSTALL_SMALL	${RAMDISK_S}	-	\
			INSTALL_TINY	${RAMDISK_T}	-	\
			INSTALL_PS2	${RAMDISK_S}	-
MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
