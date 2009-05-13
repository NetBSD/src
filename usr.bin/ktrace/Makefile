#	$NetBSD: Makefile,v 1.14.32.1 2009/05/13 19:19:54 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	ktrace
SRCS=	ktrace.c subr.c
MLINKS=	ktrace.1 ktruss.1

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=-static
.endif

.include <bsd.prog.mk>
