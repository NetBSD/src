#	$NetBSD: Makefile,v 1.7.2.3 2008/09/24 16:35:06 wrstuden Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
