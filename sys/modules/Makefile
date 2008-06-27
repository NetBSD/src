# $NetBSD: Makefile,v 1.5.4.1 2008/06/27 15:11:48 simonb Exp $

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
