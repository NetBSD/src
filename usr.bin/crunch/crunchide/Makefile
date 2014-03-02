#	$NetBSD: Makefile,v 1.19 2014/03/02 03:42:50 matt Exp $

.include <bsd.own.mk>

PROG=   crunchide
SRCS=	crunchide.c

.for ofmt in ${OBJECT_FMTS}
CPPFLAGS+=	-DNLIST_${ofmt:tu}
SRCS+=		exec_${ofmt}.c
.endfor

.include <bsd.prog.mk>
