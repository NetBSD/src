#	$NetBSD: Makefile,v 1.31 2003/03/01 13:08:42 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
