#	from: @(#)Makefile	8.1 (Berkeley) 6/10/93
#	$NetBSD: Makefile,v 1.3.16.2 2000/12/08 09:28:40 bouyer Exp $

SRCS=   boot.c conf.c
PROG=	bootsd 
LIBS=	${LIBSA} ${LIBBUG}
DPADD=	${LIBS}

.include "../Makefile.booters"

.include <bsd.prog.mk>
