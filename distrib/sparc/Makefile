#	$NetBSD: Makefile,v 1.10 2015/05/19 06:58:52 martin Exp $

SUBDIR=		instfs ramdisk .WAIT bootfs .WAIT cdroms liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
