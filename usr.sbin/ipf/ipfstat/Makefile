#	$NetBSD: Makefile,v 1.15 2002/01/24 08:24:39 martti Exp $

PROG=	ipfstat
SRCS=	fils.c parse.c kmem.c opt.c facpri.c common.c printstate.c
MAN=	ipfstat.8
DPADD=  ${LIBCURSES} ${LIBKVM}
LDADD=  -lcurses -lkvm

.include <bsd.prog.mk>
