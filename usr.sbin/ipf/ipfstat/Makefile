#	$NetBSD: Makefile,v 1.14 2001/03/13 16:30:13 christos Exp $

PROG=	ipfstat
SRCS=	fils.c parse.c kmem.c opt.c facpri.c common.c
MAN=	ipfstat.8
DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include <bsd.prog.mk>
