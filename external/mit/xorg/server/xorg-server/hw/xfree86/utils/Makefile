#	$NetBSD: Makefile,v 1.7.2.2 2008/09/18 05:18:46 wrstuden Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
