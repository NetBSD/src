#	$NetBSD: Makefile,v 1.14 2005/07/16 22:00:40 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

WARNS=3
PROG=	ktrace
SRCS=	ktrace.c subr.c
MLINKS=	ktrace.1 ktruss.1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include <bsd.prog.mk>
