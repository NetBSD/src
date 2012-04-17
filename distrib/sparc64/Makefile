#	$NetBSD: Makefile,v 1.7.36.1 2012/04/17 00:02:47 yamt Exp $

SUBDIR=		instfs .WAIT xminiroot bootfs .WAIT cdroms liveimage
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

live_image:
	${MAKEDIRTARGET} liveimage live_image

.include <bsd.subdir.mk>
