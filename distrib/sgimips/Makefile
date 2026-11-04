#	$NetBSD: Makefile,v 1.2.92.1 2026/11/04 22:37:08 perseant Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
