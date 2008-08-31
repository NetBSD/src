#	$NetBSD: Makefile,v 1.6 2008/08/31 12:52:09 chris Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
