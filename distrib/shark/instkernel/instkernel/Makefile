#	$NetBSD: Makefile,v 1.10 2002/05/02 18:02:52 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL ${RAMDISK} -

MDSET.-.post=	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
		    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
		    netbsd.tmp netbsd.tmp

MDSET_RELEASEDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
