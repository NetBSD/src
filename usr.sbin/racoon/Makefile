#	$NetBSD: Makefile,v 1.2 2000/12/29 03:16:40 itojun Exp $

.include <bsd.own.mk>

SUBDIR+=libpfkey racoon

.if ${MKSHARE} != "no" || \
	make(clean) || make(cleandir) || make(distclean) || make(obj)
SUBDIR+=conf
.endif

.include <bsd.subdir.mk>
