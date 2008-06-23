# $NetBSD: Makefile,v 1.1.2.1 2008/06/23 04:31:57 wrstuden Exp $

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
