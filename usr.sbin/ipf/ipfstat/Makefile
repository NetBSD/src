#	$NetBSD: Makefile,v 1.13.4.2 2002/02/09 16:56:43 he Exp $

PROG=	ipfstat
SRCS=	fils.c parse.c kmem.c opt.c facpri.c common.c printstate.c
MAN=	ipfstat.8
DPADD=  ${LIBCURSES} ${LIBKVM}
LDADD=  -lcurses -lkvm

.include <bsd.prog.mk>
