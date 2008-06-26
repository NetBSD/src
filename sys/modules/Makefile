# $NetBSD: Makefile,v 1.6 2008/06/26 16:10:35 kent Exp $

SUBDIR=		cd9660
SUBDIR+=	miniroot
SUBDIR+=	putter
SUBDIR+=	puffs

.if ${MACHINE} == "i386" || ${MACHINE} == "amd64"
SUBDIR+=	azalia drm i915drm
.endif
.if ${MACHINE} == "i386"
SUBDIR+=	radeondrm
.endif

.include <bsd.subdir.mk>
