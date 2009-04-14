#	$NetBSD: Makefile,v 1.15 2009/04/14 22:15:22 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	ktrace
SRCS=	ktrace.c subr.c
MLINKS=	ktrace.1 ktruss.1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include <bsd.prog.mk>
