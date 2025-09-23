#	$NetBSD: Makefile,v 1.7 2025/09/23 20:12:10 rillig Exp $

SUBDIR=		floppies stand cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
