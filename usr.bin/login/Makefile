#	@(#)Makefile	5.6 (Berkeley) 6/24/90

PROG=	login
SRCS=	login.c
CFLAGS+=-DDES
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
