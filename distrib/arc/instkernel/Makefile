#	$NetBSD: Makefile,v 1.2 2002/04/15 01:55:07 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} -

MDSET.-.suffixes=	ecoff
MDSET.-.post=		OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
			    ${OBJCOPY} -O ecoff-littlemips \
			    -R .mdebug -R .ident \
			    netbsd.tmp netbsd.ecoff && \
			    gzip -9f netbsd.ecoff

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
