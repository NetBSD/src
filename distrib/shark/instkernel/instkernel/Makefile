#	$NetBSD: Makefile,v 1.12 2002/05/08 21:44:59 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel

MDSET_POST.-=		OBJCOPY=${OBJCOPY:Q} SIZE=${SIZE:Q} \
			    ${KERNSRCDIR}/arch/arm/conf/elf2aout.sh \
			    ${.TARGET} ${.TARGET}


.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
