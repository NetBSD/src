#	$NetBSD: Makefile,v 1.16 2016/01/26 17:37:51 christos Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils.old
.endif

.if ${MKGCC} != "no"
SUBDIR+=	${EXTERNAL_GCC_SUBDIR}
.endif

.if ${MKGDB} != "no"
SUBDIR+=        gdb
.endif

.include <bsd.subdir.mk>
