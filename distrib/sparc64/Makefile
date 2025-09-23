#	$NetBSD: Makefile,v 1.9 2025/09/23 20:12:09 rillig Exp $

SUBDIR=		instfs .WAIT xminiroot bootfs .WAIT cdroms liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
