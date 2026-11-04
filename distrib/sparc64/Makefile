#	$NetBSD: Makefile,v 1.8.54.1 2026/11/04 22:37:09 perseant Exp $

SUBDIR=		instfs .WAIT xminiroot bootfs .WAIT cdroms liveimage
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

live_image: .MAKE
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
