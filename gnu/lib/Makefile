#	$NetBSD: Makefile,v 1.30 2002/09/17 23:18:32 thorpej Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libbfd libiberty
.endif

.if ${MKGCC} != "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
