#	$NetBSD: Makefile,v 1.6 2002/05/08 21:44:54 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		RAMDISK ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_SUFFIXES.-=	ecoff create-ecoff
create-ecoff=		${OBJCOPY} -O ecoff-littlemips \
			    -R .mdebug -R .ident ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
