#	$NetBSD: Makefile,v 1.1.2.2 2008/02/18 22:07:02 mjf Exp $

PROG=	devfsd
SRCS=   devfsd.c devfsd_dev.c devfsd_mount.c devfsd_rule.c
MAN=	devfsd.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil -lprop

CFLAGS=-g

WARNS=	4

.include <bsd.prog.mk>
