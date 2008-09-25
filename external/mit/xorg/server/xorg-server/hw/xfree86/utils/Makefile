#	$NetBSD: Makefile,v 1.8 2008/09/25 13:58:41 rtr Exp $

SUBDIR=	cvt gtf xorgconfig

.if ${MACHINE_ARCH} == "i386" || ${MACHINE_ARCH} == "x86_64"
SUBDIR+=ioport pcitweak scanpci
.endif

.if ${MACHINE} == "cats" || ${MACHINE} == "macppc" || \
    ${MACHINE} == "netwinder"
SUBDIR+=pcitweak scanpci
.endif

.include <bsd.subdir.mk>
