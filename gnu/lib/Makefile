#	$NetBSD: Makefile,v 1.33 2003/07/26 14:55:12 mrg Exp $

.include <bsd.own.mk>

.if ${MKBFD} != "no"
SUBDIR+= libopcodes libbfd libiberty
.endif

.if ${MKGCC} != "no"
. if ${USE_TOOLS_TOOLCHAIN} != "no"
SUBDIR+= libg2c libgcc libobjc libstdc++
. else
SUBDIR+= libg2c3 libgcc3 libobjc3 libsupc++ libstdc++-v3
. endif
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
