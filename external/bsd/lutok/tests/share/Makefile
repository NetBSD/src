# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:44 tls Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR= lutok
.endif

.include <bsd.subdir.mk>
