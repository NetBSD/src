#	$NetBSD: Makefile,v 1.5 2007/10/25 14:20:55 nisimura Exp $

SUBDIR=		ramdisk .WAIT md-kernel cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
