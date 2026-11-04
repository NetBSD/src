#       $NetBSD: Makefile,v 1.2.2.1 2026/11/04 22:37:09 perseant Exp $

SUBDIR=		installdvd

.if !defined(CDSOURCE)
SUBDIR+=	installcd
.endif

TARGETS+=	release iso_image

.include <bsd.subdir.mk>
