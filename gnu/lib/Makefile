#	$NetBSD: Makefile,v 1.38 2004/08/28 00:18:38 thorpej Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
. if ${HAVE_GCC3} == "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
. else
SUBDIR+= crtstuff3 libg2c3 libfrtbegin libgcc3 libobjc3 libsupc++ libstdc++-v3
. endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
