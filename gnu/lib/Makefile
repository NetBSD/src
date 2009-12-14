#	$NetBSD: Makefile,v 1.46 2009/12/14 12:43:48 uebayasi Exp $

.include <bsd.own.mk>

.if ${MKGCC} != "no"
SUBDIR+= crtstuff4 libgcc4 libobjc4
.if ${MKCXX} != "no"
SUBDIR+= libsupc++4 libstdc++-v3_4
.endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
