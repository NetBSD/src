#	$NetBSD: Makefile,v 1.7.2.1 2004/09/01 18:21:04 tron Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.-=	ecoff create-ecoff
create-ecoff=		${OBJCOPY} -O ecoff-littlemips \
			    -R .pdr -R .mdebug.abi32 -R .comment -R .ident \
			    ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
