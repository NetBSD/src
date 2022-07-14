#	$NetBSD: Makefile,v 1.3 2022/07/14 07:42:11 mrg Exp $

SUBDIR=	gtf

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
