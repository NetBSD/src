#	$NetBSD: Makefile,v 1.4 2008/08/30 01:05:59 mrg Exp $

SUBDIR=	cvt gtf

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

# xorgcfg xorgconfig

.include <bsd.subdir.mk>
