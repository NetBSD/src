#	$NetBSD: Makefile,v 1.2 2025/02/02 17:04:19 tsutsui Exp $

SUBDIR=		ramdisk .WAIT kernel cdroms sysnbsd
SUBDIR+=	.WAIT
SUBDIR+=	liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
