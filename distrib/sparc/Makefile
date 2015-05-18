#	$NetBSD: Makefile,v 1.9 2015/05/18 06:25:05 martin Exp $

SUBDIR=		instfs ramdisk .WAIT bootfs instkern .WAIT cdroms liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
