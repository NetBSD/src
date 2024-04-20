#	$NetBSD: Makefile.inc,v 1.19 2024/04/20 13:24:48 rillig Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR?=	/bin

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.if ${MKLINT} != "no" && !defined(NOLINT)
realall: lint
.endif
