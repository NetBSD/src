#	$NetBSD: Makefile,v 1.3.2.2 2008/01/02 11:39:02 mrg Exp $

SUBDIR=		ramdisks .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
