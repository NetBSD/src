#	@(#)Makefile	5.6 (Berkeley) 6/24/90

PROG=	login
SRCS=	login.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
.ifndef EXPORTABLE_SYSTEM
DPADD+=	${LIBCRYPT}
LDADD+= -lcrypt
.endif
BINOWN=	root
BINMODE=4555

.include <bsd.prog.mk>
