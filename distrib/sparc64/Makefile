#	$NetBSD: Makefile,v 1.7 2007/03/06 21:57:24 bouyer Exp $

SUBDIR=		instfs .WAIT xminiroot bootfs .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
