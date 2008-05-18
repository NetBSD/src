# $NetBSD: Makefile,v 1.2 2008/05/18 20:55:24 jmcneill Exp $

SUBDIR=		miniroot

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	drm i915drm
.endif

.include <bsd.subdir.mk>
