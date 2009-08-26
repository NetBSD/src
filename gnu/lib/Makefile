#	$NetBSD: Makefile,v 1.42.30.1 2009/08/26 03:37:08 matt Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
SUBDIR+= crtstuff4 libgcc4 libobjc4
.if ${MKCXX} != "no"
SUBDIR+= libsupc++4 libstdc++-v3_4
.endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
