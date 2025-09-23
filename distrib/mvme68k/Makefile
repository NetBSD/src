#	$NetBSD: Makefile,v 1.5 2025/09/23 20:12:08 rillig Exp $

SUBDIR=		ramdisk .WAIT instkernel cdroms
TARGETS+=	release

iso_image: .MAKE
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
