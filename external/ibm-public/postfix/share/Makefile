#	$NetBSD: Makefile,v 1.1.4.2 2010/04/21 05:24:32 matt Exp $

.include <bsd.own.mk>

SUBDIR=	README_FILES html

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif


.include <bsd.subdir.mk>
