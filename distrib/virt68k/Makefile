#	$NetBSD: Makefile,v 1.1.2.2 2026/11/04 22:37:12 perseant Exp $

.include <bsd.own.mk>

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	liveimage

TARGETS+=	release

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
