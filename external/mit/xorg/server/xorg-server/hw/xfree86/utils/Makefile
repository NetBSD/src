#	$NetBSD: Makefile,v 1.7 2008/09/12 07:47:11 mrg Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
