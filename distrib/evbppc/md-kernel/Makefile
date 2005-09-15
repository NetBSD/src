#	$NetBSD: Makefile,v 1.1.4.2 2005/09/15 14:28:44 riz Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKIMG?=	${HOST_SH} ${NETBSDSRCDIR}/sys/arch/evbppc/compile/walnut-mkimg.sh

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL_WALNUT ${RAMDISK} -	\
			INSTALL_OPENBLOCKS266 ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel
MDSET_SUFFIXES.-=	img create-img

create-img= \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; export OBJDUMP OBJCOPY; \
		${MKIMG} ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
