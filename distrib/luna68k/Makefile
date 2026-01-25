#	$NetBSD: Makefile,v 1.1.58.1 2026/01/25 16:31:07 martin Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
SUBDIR+=	liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
