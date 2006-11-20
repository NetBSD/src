#	$NetBSD: Makefile,v 1.42 2006/11/20 17:08:35 skrll Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
SUBDIR+= crtstuff4 libgcc4 libobjc4 libsupc++4 libstdc++-v3_4
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
