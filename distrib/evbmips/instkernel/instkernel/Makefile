#	$NetBSD: Makefile,v 1.8 2021/04/25 23:40:05 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=		cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=		${RAMDISKDIR}/ramdisk.fs

.if ${MACHINE_ARCH} == "mipsel"
# Little endian platforms.
MDSETTARGETS=		INSTALL_CI20		${RAMDISK}	-
MDSETTARGETS+=		INSTALL_OMSAL400	${RAMDISK}	-
MDSETTARGETS+=		INSTALL_MALTA		${RAMDISK}	-

.elif ${MACHINE_MIPS64}
MDSETTARGETS=		INSTALL_MALTA32		${RAMDISK}	-
MDSETTARGETS+=		INSTALL_MALTA64		${RAMDISK}	-
MDSETTARGETS+=		INSTALL_XLSATX32	${RAMDISK}	-
MDSETTARGETS+=		INSTALL_XLSATX64	${RAMDISK}	-
.if !empty(MACHINE_ARCH:Mmips*64el)
MDSETTARGETS+=		INSTALL_LOONGSON	${RAMDISK}	-
.else
MDSETTARGETS+=		INSTALL_OCTEON		${RAMDISK}	-
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
