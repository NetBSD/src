#	$NetBSD: Makefile,v 1.4 2004/01/11 23:44:31 lukem Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=fonts man nls
.endif

.include <bsd.subdir.mk>
