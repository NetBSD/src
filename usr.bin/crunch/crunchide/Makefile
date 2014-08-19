#	$NetBSD: Makefile,v 1.17.12.1 2014/08/20 00:04:57 tls Exp $

.include <bsd.own.mk>

PROG=   crunchide
SRCS=	crunchide.c

.for ofmt in ${OBJECT_FMTS}
CPPFLAGS+=	-DNLIST_${ofmt:tu}
SRCS+=		exec_${ofmt}.c
.endfor

.include <bsd.prog.mk>
