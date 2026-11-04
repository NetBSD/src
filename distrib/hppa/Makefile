#	$NetBSD: Makefile,v 1.1.42.2 2026/11/04 22:36:54 perseant Exp $

SUBDIR=		ramdisk .WAIT kernel cdroms sysnbsd
SUBDIR+=	.WAIT
SUBDIR+=	liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
