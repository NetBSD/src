#	$NetBSD: Makefile,v 1.5 2002/05/03 15:22:57 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	RAMDISK ${RAMDISK} -

MDSET.-.suffixes=	ecoff
MDSET.-.post=		OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
			    ${OBJCOPY} -O ecoff-littlemips \
			    -R .mdebug -R .ident \
			    ${.TARGET:R} ${.TARGET:R}.ecoff && \
			gzip -9f ${.TARGET:R}.ecoff

MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
