#	$NetBSD: Makefile,v 1.10.2.1 2009/05/13 18:53:29 jym Exp $

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
