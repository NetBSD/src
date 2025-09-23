#	$NetBSD: Makefile,v 1.3 2025/09/23 20:12:07 rillig Exp $

SUBDIR=		netboot cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
