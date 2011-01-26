#	$NetBSD: Makefile,v 1.1 2011/01/26 01:18:43 pooka Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
