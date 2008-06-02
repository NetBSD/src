# $NetBSD: Makefile,v 1.5.2.2 2008/06/02 13:24:21 mjf Exp $

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
