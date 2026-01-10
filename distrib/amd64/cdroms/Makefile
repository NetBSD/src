#	$NetBSD: Makefile,v 1.6 2026/01/10 23:59:50 nia Exp $

SUBDIR=
SUBDIR+=	bootcd
SUBDIR+=	bootcd-com
SUBDIR+=	installdvd

.include <bsd.own.mk>

.if !defined(CDSOURCE) && ${USE_XZ_SETS:Uno:tl} != "no"
SUBDIR+=	installcd
.endif

TARGETS+=       release iso_image

.include <bsd.subdir.mk>
