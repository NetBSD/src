#	$NetBSD: Makefile.inc,v 1.18 2021/10/09 21:06:31 rillig Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR?=	/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.if ${MKLINT} != "no" && ${LINT_SUPPORTED:Uyes} == "yes"
realall: lint
.endif
