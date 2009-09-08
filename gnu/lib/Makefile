#	$NetBSD: Makefile,v 1.44 2009/09/08 07:08:02 skrll Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
.if ${HAVE_BINUTILS} == "216"
SUBDIR+= libopcodes libbfd libiberty
.endif
.endif

.if ${MKGCC} != "no"
SUBDIR+= crtstuff4 libgcc4 libobjc4 libsupc++4 libstdc++-v3_4
.endif

SUBDIR+= libmalloc

.include <bsd.subdir.mk>
