#	$NetBSD: Makefile,v 1.2.18.1 2007/03/09 06:59:46 riz Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
