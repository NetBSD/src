#	$NetBSD: Makefile.inc,v 1.23 2021/09/14 20:13:03 rillig Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 6/8/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR?=	/sbin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.if ${MKLINT} != "no" && ${LINT_SUPPORTED:Uyes} == "yes"
realall: lint
.endif
