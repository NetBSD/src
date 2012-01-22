#	$NetBSD: Makefile,v 1.8 2012/01/22 03:53:33 tsutsui Exp $

SUBDIR=		instfs ramdisk .WAIT bootfs .WAIT cdroms liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
