#	$NetBSD: Makefile,v 1.1.42.1 2025/08/02 05:20:21 perseant Exp $

SUBDIR=		ramdisk .WAIT kernel cdroms sysnbsd
SUBDIR+=	.WAIT
SUBDIR+=	liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
