#	$NetBSD: Makefile,v 1.3 2003/09/12 18:38:31 lukem Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	cf fonts man nls
.endif

.include <bsd.subdir.mk>
