#	$NetBSD: Makefile,v 1.2 2002/04/12 01:15:14 briggs Exp $

SUBDIR=	floppies

.include <bsd.subdir.mk>

release: check_RELEASEDIR
	cd ${.CURDIR}/floppies/md-kernel && ${MAKE} release
