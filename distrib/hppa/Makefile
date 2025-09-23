#	$NetBSD: Makefile,v 1.3 2025/09/23 20:12:07 rillig Exp $

SUBDIR=		ramdisk .WAIT kernel cdroms sysnbsd
SUBDIR+=	.WAIT
SUBDIR+=	liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
