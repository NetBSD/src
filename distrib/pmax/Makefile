#	$NetBSD: Makefile,v 1.5 2012/01/22 03:53:32 tsutsui Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
