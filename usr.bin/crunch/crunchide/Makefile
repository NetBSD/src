#	$NetBSD: Makefile,v 1.17.6.1 2014/05/22 11:42:43 yamt Exp $

.include <bsd.own.mk>

PROG=   crunchide
SRCS=	crunchide.c

.for ofmt in ${OBJECT_FMTS}
CPPFLAGS+=	-DNLIST_${ofmt:tu}
SRCS+=		exec_${ofmt}.c
.endfor

.include <bsd.prog.mk>
