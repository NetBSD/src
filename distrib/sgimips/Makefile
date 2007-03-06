#	$NetBSD: Makefile,v 1.2 2007/03/06 21:57:21 bouyer Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
