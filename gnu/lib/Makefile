#	$NetBSD: Makefile,v 1.34 2003/08/27 16:03:17 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
. if ${HAVE_GCC3} == "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
. else
SUBDIR+= libg2c3 libgcc3 libobjc3 libsupc++ libstdc++-v3
. endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
