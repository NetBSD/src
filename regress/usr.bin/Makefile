#	$NetBSD: Makefile,v 1.15.18.1 2009/08/26 03:34:33 matt Exp $

.include <bsd.own.mk>

SUBDIR+= basename bzip2
.if ${MKCXX} != "no"
SUBDIR+= c++
.endif
SUBDIR+= config cut dirname grep gzip m4 make rtld
SUBDIR+= sdiff sort xlint

.include <bsd.subdir.mk>
