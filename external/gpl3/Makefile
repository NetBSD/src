#	$NetBSD: Makefile,v 1.3 2011/06/21 04:52:49 mrg Exp $

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
