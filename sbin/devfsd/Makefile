#	$NetBSD: Makefile,v 1.1.2.1 2007/12/08 22:05:04 mjf Exp $

PROG=	devfsd
SRCS=   devfsd.c devfsd_dev.c devfsd_rule.c
MAN=	devfsd.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil

WARNS=	4

.include <bsd.prog.mk>
