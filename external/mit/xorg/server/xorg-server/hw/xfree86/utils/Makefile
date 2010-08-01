#	$NetBSD: Makefile,v 1.14 2010/08/01 06:24:31 kiyohara Exp $

SUBDIR=	cvt gtf
#SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
#SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder" || ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE} == "prep" || ${MACHINE} == "bebox"
#SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
