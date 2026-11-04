#	$NetBSD: Makefile,v 1.4.92.1 2026/11/04 22:36:56 perseant Exp $

SUBDIR=		ramdisk .WAIT instkernel cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
