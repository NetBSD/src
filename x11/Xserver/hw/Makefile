#	$NetBSD: Makefile,v 1.5 2003/12/27 15:16:09 mrg Exp $

SUBDIR=	vfb xnest

.if ${MACHINE} == "sparc" || ${MACHINE} == "sparc64"
SUBDIR+=sun
.endif

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64" || \
    ${MACHINE} == "macppc" || ${MACHINE} == "cats"
SUBDIR+=xfree86
.endif

.if ${MACHINE} == "macppc"
SUBDIR+=netbsd/macppc
.endif


.include <bsd.subdir.mk>
