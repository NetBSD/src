#	$NetBSD: Makefile,v 1.41 2006/06/02 22:16:18 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
. if ${HAVE_GCC} == "3"
SUBDIR+= crtstuff3 libg2c3 libfrtbegin libgcc3 libobjc3 libsupc++ libstdc++-v3
. else
SUBDIR+= crtstuff4 libgcc4 libobjc4 libsupc++4 libstdc++-v3_4
. endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
