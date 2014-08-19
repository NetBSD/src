#	$NetBSD: Makefile,v 1.1.8.2 2014/08/19 23:45:40 tls Exp $

SUBDIR=	ramdisk .WAIT kernel cdroms sysnbsd
TARGETS+=release

iso_image:
	${MAKEDIRTARGET} cdroms iso_image

.include <bsd.subdir.mk>
