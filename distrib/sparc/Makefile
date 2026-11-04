#	$NetBSD: Makefile,v 1.10.28.1 2026/11/04 22:37:08 perseant Exp $

SUBDIR=		instfs ramdisk .WAIT bootfs .WAIT cdroms liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
