#	$NetBSD: Makefile,v 1.7 2002/04/12 13:39:49 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL ${RAMDISK} -

netbsd.INSTALL.post=	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
			    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
			    netbsd.tmp netbsd.tmp

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
