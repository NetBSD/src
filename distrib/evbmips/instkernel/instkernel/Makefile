#	$NetBSD: Makefile,v 1.1 2006/03/30 15:45:42 shige Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=		cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=		${RAMDISKDIR}/ramdisk.fs

.if ${MACHINE_ARCH} == "mipsel"
# Little endian platforms.
MDSETTARGETS=		INSTALL_OMSAL400	${RAMDISK}	-
.else
# Big endian platforms.
MDSETTARGETS=		
.endif
MDSET_RELEASEDIR=	installation
MDSET_SUFFIXES.-=	srec create-srec

create-srec=		${OBJCOPY} -O srec ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
