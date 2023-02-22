#	$NetBSD: Makefile,v 1.2.2.3 2023/02/22 12:59:17 martin Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
