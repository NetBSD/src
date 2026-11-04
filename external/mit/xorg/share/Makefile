#	$NetBSD: Makefile,v 1.2.58.1 2026/11/04 23:15:29 perseant Exp $

.include <bsd.own.mk>

SUBDIR=	cf
.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=examples fonts man nls
.endif

.include <bsd.subdir.mk>
