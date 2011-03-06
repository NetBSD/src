#	$NetBSD: Makefile,v 1.1.2.2 2011/03/06 15:07:36 bouyer Exp $

SUBDIR=		ramdisk .WAIT instkernel .WAIT cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
