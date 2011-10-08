#	$NetBSD: Makefile,v 1.5 2011/10/08 21:52:15 christos Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "219"
SUBDIR+=	binutils
.endif

.if ${HAVE_GCC} == 45
.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc
.endif
.endif

.if ${MKGDB} != "no"
.if ${HAVE_GDB} == "7"
SUBDIR+=        gdb
.endif
.endif

.include <bsd.subdir.mk>
