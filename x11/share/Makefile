#	$NetBSD: Makefile,v 1.2 2003/09/12 16:27:29 lukem Exp $

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR=	cf fonts man nls
.endif

.include <bsd.subdir.mk>
