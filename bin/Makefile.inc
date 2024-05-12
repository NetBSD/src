#	$NetBSD: Makefile.inc,v 1.20 2024/05/12 17:55:36 rillig Exp $
#	@(#)Makefile.inc	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS?=		5
BINDIR?=	/bin

LINTFLAGS+=	-u	# don't warn about "used but not defined"

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.if ${MKLINT} != "no" && !defined(NOLINT)
realall: lint
.endif
