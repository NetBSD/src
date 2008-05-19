# $NetBSD: Makefile,v 1.4 2008/05/19 20:22:17 jmcneill Exp $

SUBDIR=		miniroot
SUBDIR+=	putter
SUBDIR+=	puffs

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	drm i915drm
.endif
.if ${MACHINE} == "i386"
SUBDIR+=	radeondrm
.endif

.include <bsd.subdir.mk>
