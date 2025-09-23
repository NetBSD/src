#	$NetBSD: Makefile,v 1.3 2025/09/23 20:12:10 rillig Exp $

.include <bsd.own.mk>

SUBDIR=
SUBDIR+=	ramdisks
SUBDIR+=	.WAIT
SUBDIR+=	liveimage

TARGETS+=	release

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
