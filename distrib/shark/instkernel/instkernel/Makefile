#	$NetBSD: Makefile,v 1.11 2002/05/03 15:22:57 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL ${RAMDISK} -

MDSET.-.post=	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
		    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
		    ${.TARGET:R} ${.TARGET:R}

MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
