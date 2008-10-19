#	$NetBSD: Makefile,v 1.1.1.1.6.2 2008/10/19 22:41:22 haad Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=fonts nls
.endif

.include <bsd.subdir.mk>
