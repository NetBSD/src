#	$NetBSD: Makefile,v 1.6 2002/04/12 05:34:02 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
TARGETS=	INSTALL ${RAMDISK}

netbsd.INSTALL.post=	OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
			    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
			    netbsd.tmp netbsd.tmp

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
