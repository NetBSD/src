#	$NetBSD: Makefile,v 1.3.84.1 2026/11/04 22:36:57 perseant Exp $

SUBDIR=		ramdisks .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
