#	$NetBSD: Makefile,v 1.11 2005/02/09 14:21:37 xtraeme Exp $

PROG=	raidctl 
SRCS=	rf_configure.c raidctl.c
MAN=	raidctl.8

WARNS=	3

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
