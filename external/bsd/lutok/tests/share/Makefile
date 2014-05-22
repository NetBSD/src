# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:49:34 yamt Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR= lutok
.endif

.include <bsd.subdir.mk>
