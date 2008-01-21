#	$NetBSD: Makefile,v 1.1.4.2 2008/01/21 19:36:59 bouyer Exp $

SUBDIR=	ramdisk .WAIT kernel cdroms sysnbsd
TARGETS+=release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
