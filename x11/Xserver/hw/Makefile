#	$NetBSD: Makefile,v 1.6 2004/01/03 02:05:49 takemura Exp $

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

.if ${MACHINE} == "hpcmips"
SUBDIR+=netbsd/hpc
.endif


.include <bsd.subdir.mk>
