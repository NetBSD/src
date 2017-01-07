#	$NetBSD: Makefile,v 1.1.1.1.2.1 2017/01/07 08:56:01 pgoyette Exp $

SUBDIR=	cvt gtf

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
#SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder" || ${MACHINE_ARCH} == "sparc" || \
    ${MACHINE_ARCH} == "sparc64" || ${MACHINE_ARCH} == "alpha" || \
    ${MACHINE} == "prep" || ${MACHINE} == "bebox" || \
    ${MACHINE} == "ofppc"
#SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
