#	$NetBSD: Makefile,v 1.1.60.1 2026/11/04 22:36:52 perseant Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
