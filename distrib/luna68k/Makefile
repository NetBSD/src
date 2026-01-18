#	$NetBSD: Makefile,v 1.3 2026/01/18 00:12:10 tsutsui Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
SUBDIR+=	liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
