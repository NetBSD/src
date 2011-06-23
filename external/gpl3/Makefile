#	$NetBSD: Makefile,v 1.2.4.1 2011/06/23 14:18:33 cherry Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "219"
SUBDIR+=	binutils
.endif

.if ${HAVE_GCC} == 45
.if ${MKGCCCMDS} != "no"
#SUBDIR+=	gcc
.endif
.endif

.include <bsd.subdir.mk>
