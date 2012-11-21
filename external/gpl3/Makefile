#	$NetBSD: Makefile,v 1.6.6.1 2012/11/21 01:04:34 matt Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no"
.if ${HAVE_BINUTILS} == "219"
SUBDIR+=	binutils
.endif
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
