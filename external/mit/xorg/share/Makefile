#	$NetBSD: Makefile,v 1.3 2025/11/17 00:14:23 nia Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=examples fonts man nls
.endif

.include <bsd.subdir.mk>
