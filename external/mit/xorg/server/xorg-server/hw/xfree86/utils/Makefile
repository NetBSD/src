#	$NetBSD: Makefile,v 1.8.4.2 2008/10/19 22:41:20 haad Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
