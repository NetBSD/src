#	$NetBSD: Makefile,v 1.39 2006/04/07 19:38:58 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
. if ${HAVE_GCC} == "2"
SUBDIR+= libg2c libgcc libobjc libstdc++
. elif ${HAVE_GCC} == "3"
SUBDIR+= crtstuff3 libg2c3 libfrtbegin libgcc3 libobjc3 libsupc++ libstdc++-v3
. else
SUBDIR+= crtstuff4 libgcc4 libobjc4 libsupc++4 libstdc++-v34
. endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
