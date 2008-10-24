#	$NetBSD: Makefile,v 1.9 2008/10/24 19:48:34 macallan Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder" || ${MACHINE} == "sparc" || \
    ${MACHINE} == "sparc64"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
