#	$NetBSD: Makefile,v 1.5.2.1 2026/11/04 22:36:54 perseant Exp $

SUBDIR=
SUBDIR+=	bootcd
SUBDIR+=	bootcd-com
SUBDIR+=	installdvd

.if !defined(CDSOURCE)
SUBDIR+=	installcd
.endif

TARGETS+=       release iso_image

.include <bsd.subdir.mk>
