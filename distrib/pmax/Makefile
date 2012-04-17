#	$NetBSD: Makefile,v 1.4.36.1 2012/04/17 00:02:37 yamt Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
