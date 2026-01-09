#       $NetBSD: Makefile,v 1.3 2026/01/09 19:46:57 nia Exp $

SUBDIR=		installdvd

.if !defined(CDSOURCE)
SUBDIR+=	installcd
.endif

TARGETS+=	release iso_image

.include <bsd.subdir.mk>
