#	$NetBSD: Makefile,v 1.13.4.1 2001/07/29 19:29:37 he Exp $

PROG=	ipfstat
SRCS=	fils.c parse.c kmem.c opt.c facpri.c common.c
MAN=	ipfstat.8
DPADD=  ${LIBCURSES}
LDADD=  -lcurses

.include <bsd.prog.mk>
