#	$NetBSD: Makefile,v 1.6.18.1 2007/03/09 06:59:21 riz Exp $

SUBDIR=		instfs ramdisk .WAIT bootfs .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
