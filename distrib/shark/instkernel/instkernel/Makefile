#	$NetBSD: Makefile,v 1.9 2002/04/26 15:50:46 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL ${RAMDISK} -

MDSET.-.post=	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
		    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
		    netbsd.tmp netbsd.tmp

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
