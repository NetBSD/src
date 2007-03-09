#	$NetBSD: Makefile,v 1.6.18.1 2007/03/09 06:59:42 riz Exp $

SUBDIR=		instfs .WAIT xminiroot bootfs .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
