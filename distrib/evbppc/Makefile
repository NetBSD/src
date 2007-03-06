#	$NetBSD: Makefile,v 1.2 2007/03/06 21:56:59 bouyer Exp $

SUBDIR=		ramdisk .WAIT md-kernel cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
