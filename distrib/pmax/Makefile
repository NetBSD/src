#	$NetBSD: Makefile,v 1.4 2007/03/06 21:57:15 bouyer Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
