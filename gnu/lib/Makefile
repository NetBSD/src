#	$NetBSD: Makefile,v 1.47 2010/11/28 18:40:55 skrll Exp $

.include <bsd.own.mk>

.if ${MKGCC} != "no"
.if ${USE_COMPILERCRTSTUFF} == "yes"
SUBDIR+= crtstuff4
.endif
SUBDIR+= libgcc4 libobjc4
.if ${MKCXX} != "no"
SUBDIR+= libsupc++4 libstdc++-v3_4
.endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
