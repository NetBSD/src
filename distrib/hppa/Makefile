#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 12:01:29 yamt Exp $

SUBDIR=	ramdisk .WAIT kernel cdroms sysnbsd
TARGETS+=release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
