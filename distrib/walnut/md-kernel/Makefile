#	$NetBSD: Makefile,v 1.1.2.3 2003/08/12 18:28:33 tron Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKIMG?=	${NETBSDSRCDIR}/sys/arch/walnut/compile/mkimg.sh

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel
MDSET_SUFFIXES.-=	img create-img

create-img= \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		sh ${MKIMG} ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
