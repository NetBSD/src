#	$NetBSD: Makefile,v 1.1 2009/06/25 18:22:01 tron Exp $

.include <bsd.own.mk>

SUBDIR=	README_FILES html

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif


.include <bsd.subdir.mk>
