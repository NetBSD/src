#	$NetBSD: Makefile,v 1.3.2.2 2008/01/06 20:16:25 skrll Exp $

SUBDIR=	ramdisk .WAIT kernel cdroms sysnbsd
TARGETS+=release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
