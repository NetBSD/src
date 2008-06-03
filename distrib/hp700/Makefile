#	$NetBSD: Makefile,v 1.1.4.1.2.1 2008/06/03 20:46:57 skrll Exp $

SUBDIR=	ramdisk .WAIT kernel cdroms sysnbsd
TARGETS+=release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
