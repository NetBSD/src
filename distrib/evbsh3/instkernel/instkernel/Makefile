#	$NetBSD: Makefile,v 1.1 2010/04/06 15:54:31 nonaka Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

.if ${MACHINE_ARCH} == "sh3eb"
# Big endian platforms.
MDSETTARGETS=
.endif
.if ${MACHINE_ARCH} == "sh3el"
# Little endian platforms.
MDSETTARGETS=		AP_MS104_SH4_INSTALL	${RAMDISK}	-	\
			T_SH7706LAN_INSTALL	${RAMDISK}	-	\
			T_SH7706LSR_INSTALL	${RAMDISK}	-	
.endif
MDSET_RELEASEDIR=	installation/instkernel

MDSET_SUFFIXES.-=	bin create-bin
create-bin=		${OBJCOPY} -O binary ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
