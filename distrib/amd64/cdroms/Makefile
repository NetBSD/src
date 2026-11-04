#	$NetBSD: Makefile,v 1.4.2.1 2026/11/04 22:36:51 perseant Exp $

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
