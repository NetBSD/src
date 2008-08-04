#	$NetBSD: Makefile,v 1.3 2008/08/04 04:34:23 mrg Exp $

SUBDIR=	cvt gtf

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "amd64"
SUBDIR+=ioport pcitweak scanpci
.endif

# xorgcfg xorgconfig

.include <bsd.subdir.mk>
