#	from: @(#)Makefile	8.1 (Berkeley) 7/19/93
#	$Id: Makefile,v 1.14 1994/06/11 07:54:01 mycroft Exp $

PROG=	init
MAN8=	init.0
DPADD=	${LIBUTIL} ${LIBCRYPT}
LDADD=	-lutil -lcrypt
BINMODE=500
INSTALLFLAGS=-fschg
CFLAGS+=-DDEBUGSHELL -DSECURE

.include <bsd.prog.mk>
