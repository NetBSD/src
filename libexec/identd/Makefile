#	$Id: Makefile,v 1.1.1.1 1994/02/04 21:41:32 cgd Exp $

PROG=	identd
SRCS=	config.c identd.c netbsd.c parse.c proxy.c version.c
MAN8=	identd.0

LDADD=	-lkvm
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
