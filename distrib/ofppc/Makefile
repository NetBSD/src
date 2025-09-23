#	$NetBSD: Makefile,v 1.4 2025/09/23 20:12:08 rillig Exp $

SUBDIR=		ramdisks .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
