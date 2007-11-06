#	$NetBSD: Makefile,v 1.4.4.1 2007/11/06 23:10:00 matt Exp $

SUBDIR=		ramdisk .WAIT md-kernel cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
