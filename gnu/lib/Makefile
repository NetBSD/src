#	$NetBSD: Makefile,v 1.36 2003/11/27 13:53:11 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
. if ${HAVE_BINUTILS214} == "no"
SUBDIR+= libopcodes libbfd libiberty
. endif
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
