#	$NetBSD: Makefile,v 1.2.4.1 2008/01/09 01:27:52 matt Exp $

SUBDIR=	ramdisk .WAIT kernel cdroms sysnbsd
TARGETS+=release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
