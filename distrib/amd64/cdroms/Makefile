#	$NetBSD: Makefile,v 1.5 2026/01/09 19:46:56 nia Exp $

SUBDIR=
SUBDIR+=	bootcd
SUBDIR+=	bootcd-com
SUBDIR+=	installdvd

.if !defined(CDSOURCE)
SUBDIR+=	installcd
.endif

TARGETS+=       release iso_image

.include <bsd.subdir.mk>
