#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.6.152.1 2011/03/05 20:51:18 rmind Exp $

SRCS=   boot.c conf.c
PROG=	bootsd 
LIBS=	${LIBSA} ${LIBKERN} ${LIBBUG}
DPADD=	${LIBS}

.include "../Makefile.booters"

.include <bsd.prog.mk>
