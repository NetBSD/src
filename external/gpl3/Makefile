#	$NetBSD: Makefile,v 1.4 2011/06/29 02:05:24 mrg Exp $

.include <bsd.own.mk>

.if ${MKBINUTILS} != "no" && ${HAVE_BINUTILS} == "219"
SUBDIR+=	binutils
.endif

.if ${HAVE_GCC} == 45
.if ${MKGCCCMDS} != "no"
SUBDIR+=	gcc
.endif
.endif

.include <bsd.subdir.mk>
