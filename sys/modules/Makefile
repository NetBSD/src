# $NetBSD: Makefile,v 1.1.6.3 2008/06/04 02:05:47 yamt Exp $

SUBDIR=		cd9660
SUBDIR+=	miniroot
SUBDIR+=	putter
SUBDIR+=	puffs

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	drm i915drm
.endif
.if ${MACHINE} == "i386"
SUBDIR+=	radeondrm
.endif

.include <bsd.subdir.mk>
