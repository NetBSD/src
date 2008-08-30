#	$NetBSD: Makefile,v 1.5 2008/08/30 06:36:33 mrg Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.include <bsd.subdir.mk>
