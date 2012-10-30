#	$NetBSD: Makefile,v 1.6.2.1 2012/10/30 18:56:39 yamt Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
SUBDIR+=	binutils
.endif

.if ${MKGCC} != "no"
.if ${HAVE_GCC} == 45
.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc
.endif
.endif
.endif

.if ${MKGDB} != "no"
.if ${HAVE_GDB} == "7"
SUBDIR+=        gdb
.endif
.endif

.include <bsd.subdir.mk>
