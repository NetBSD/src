#	$NetBSD: Makefile,v 1.1.1.1.2.2 2008/09/18 05:18:48 wrstuden Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=fonts nls
.endif

.include <bsd.subdir.mk>
