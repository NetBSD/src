#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.6 2000/12/04 21:25:58 scw Exp $

SRCS=   boot.c conf.c
PROG=	bootsd 
LIBS=	${LIBSA} ${LIBBUG}
DPADD=	${LIBS}

.include "../Makefile.booters"

.include <bsd.prog.mk>
