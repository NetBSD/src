#	$NetBSD: Makefile,v 1.11 2009/02/14 13:32:13 abs Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder" || ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "alpha"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
