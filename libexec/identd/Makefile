#	$Id: Makefile,v 1.2 1994/12/22 10:27:12 cgd Exp $

PROG=	identd
SRCS=	config.c identd.c netbsd.c parse.c proxy.c version.c
MAN=	identd.8

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
