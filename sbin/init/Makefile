#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93
#	$Id: Makefile,v 1.15 1994/12/22 10:45:16 cgd Exp $

PROG=	init
MAN=	init.8
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500
INSTALLFLAGS=-fschg
CFLAGS+=-DDEBUGSHELL -DSECURE

.include <bsd.prog.mk>
