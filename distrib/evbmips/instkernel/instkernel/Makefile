#	$NetBSD: Makefile,v 1.5 2015/04/06 14:18:09 joerg Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=		cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=		${RAMDISKDIR}/ramdisk.fs

.if ${MACHINE_ARCH} == "mipsel"
# Little endian platforms.
MDSETTARGETS=		INSTALL_CI20		${RAMDISK}	-
MDSETTARGETS+=		INSTALL_OMSAL400	${RAMDISK}	-
MDSETTARGETS+=		INSTALL_MALTA		${RAMDISK}	-

.elif ${MACHINE_ARCH} == "mips64eb" || ${MACHINE_ARCH} == "mips64el"
MDSETTARGETS=		INSTALL_MALTA32		${RAMDISK}	-
MDSETTARGETS+=		INSTALL_MALTA64		${RAMDISK}	-
MDSETTARGETS+=		INSTALL_XLSATX32	${RAMDISK}	-
MDSETTARGETS+=		INSTALL_XLSATX64	${RAMDISK}	-
.if ${MACHINE_ARCH} == "mips64el"
MDSETTARGETS+=		INSTALL_LOONGSON	${RAMDISK}	-
.endif

.else
# Big endian platforms.
MDSETTARGETS=		INSTALL_MALTA		${RAMDISK}	-

.endif
MDSET_RELEASEDIR=	installation
MDSET_SUFFIXES.-=	srec create-srec

create-srec=		${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
