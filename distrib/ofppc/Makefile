#	$NetBSD: Makefile,v 1.2.4.1 2008/01/09 01:28:07 matt Exp $

SUBDIR=		ramdisks .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
