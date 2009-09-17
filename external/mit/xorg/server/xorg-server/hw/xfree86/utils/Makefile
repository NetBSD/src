#	$NetBSD: Makefile,v 1.9.2.1 2009/09/17 04:24:40 snj Exp $

SUBDIR=	cvt gtf
#SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
#SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder" || ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "alpha"
#SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
