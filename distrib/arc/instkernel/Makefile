#	$NetBSD: Makefile,v 1.3 2002/04/26 15:50:25 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

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
