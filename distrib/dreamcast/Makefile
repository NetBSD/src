#	$NetBSD: Makefile,v 1.3 2025/09/23 20:12:06 rillig Exp $

SUBDIR=		ramdisk .WAIT instkernel cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
