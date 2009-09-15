#	$NetBSD: Makefile,v 1.1.2.2 2009/09/15 06:04:19 snj Exp $

.include <bsd.own.mk>

SUBDIR=	README_FILES html

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif


.include <bsd.subdir.mk>
