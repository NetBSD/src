#	$NetBSD: Makefile,v 1.5 2002/04/14 01:41:24 lukem Exp $

.include <bsd.own.mk>
.include "${_SRC_TOP_}/distrib/Makefile.inc"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs
MDSETTARGETS=	INSTALL ${RAMDISK} netbsd.aout-INSTALL

MDSET.netbsd.aout-INSTALL.post=	\
		OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
	    	    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
		    netbsd.tmp netbsd.tmp

MDSETDIR=	binary/kernel

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
