#	$NetBSD: Makefile,v 1.1.1.1 2008/07/29 05:01:23 mrg Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=fonts nls
.endif

.include <bsd.subdir.mk>
