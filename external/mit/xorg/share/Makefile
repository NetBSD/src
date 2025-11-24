#	$NetBSD: Makefile,v 1.2.56.1 2025/11/24 16:48:30 martin Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=examples fonts man nls
.endif

.include <bsd.subdir.mk>
