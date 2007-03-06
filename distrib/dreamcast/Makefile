#	$NetBSD: Makefile,v 1.2 2007/03/06 21:56:56 bouyer Exp $

SUBDIR=		ramdisk .WAIT instkernel cdroms
TARGETS+=	release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
