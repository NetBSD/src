#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.7 2011/01/02 05:30:11 tsutsui Exp $

SRCS=   boot.c conf.c
PROG=	bootsd 
LIBS=	${LIBSA} ${LIBKERN} ${LIBBUG}
DPADD=	${LIBS}

.include "../Makefile.booters"

.include <bsd.prog.mk>
