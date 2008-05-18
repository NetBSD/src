# $NetBSD: Makefile,v 1.3 2008/05/18 21:14:04 jmcneill Exp $

SUBDIR=		miniroot

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	drm i915drm
.endif
.if ${MACHINE} == "i386"
SUBDIR+=	radeondrm
.endif

.include <bsd.subdir.mk>
