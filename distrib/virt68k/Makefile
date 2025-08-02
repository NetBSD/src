#	$NetBSD: Makefile,v 1.1.2.1 2025/08/02 05:20:41 perseant Exp $

.include <bsd.own.mk>

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	liveimage

TARGETS+=	release

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
