#	$NetBSD: Makefile,v 1.4.2.1 2014/08/10 06:48:02 tls Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

MKIMG?=	${HOST_SH} ${NETBSDSRCDIR}/sys/arch/evbppc/compile/walnut-mkimg.sh

RAMDISKDIR!=	cd ${.CURDIR}/../ramdisk && ${PRINTOBJDIR}
RAMDISK=	${RAMDISKDIR}/ramdisk.fs

MDSETTARGETS=		INSTALL_OPENBLOCKS266 ${RAMDISK} - \
			INSTALL_TWRP1025 ${RAMDISK} - \
			INSTALL_WALNUT ${RAMDISK} -
MDSET_RELEASEDIR=	binary/kernel
MDSET_SUFFIXES.-=	img create-img

create-img= \
	OBJDUMP=${OBJDUMP}; OBJCOPY=${OBJCOPY}; STAT=${TOOL_STAT}; \
		export OBJDUMP OBJCOPY STAT; ${MKIMG} ${.TARGET:R} ${.TARGET}

.include "${DISTRIBDIR}/common/Makefile.mdset"

.include <bsd.prog.mk>
