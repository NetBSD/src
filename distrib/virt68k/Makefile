#	$NetBSD: Makefile,v 1.2 2025/01/19 04:58:42 tsutsui Exp $

.include <bsd.own.mk>

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	liveimage

TARGETS+=	release

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
