#	$NetBSD: Makefile,v 1.35 2003/08/29 01:57:04 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
. if ${HAVE_GCC3} == "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
. else
SUBDIR+= libg2c3 libfrtbegin libgcc3 libobjc3 libsupc++ libstdc++-v3
. endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
