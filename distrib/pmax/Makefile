#	$NetBSD: Makefile,v 1.5.54.1 2026/11/04 22:36:57 perseant Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
