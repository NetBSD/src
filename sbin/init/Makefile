#	$Id: Makefile,v 1.6 1993/06/10 01:02:59 cgd Exp $

PROG=	init
#SRCS=	init.c 
CFLAGS+= -DSECURE_CONSOLE -DUSE_DEVFS
DPADD=	${LIBUTIL}
LDADD=	-lutil
NOMAN=noman
LDFLAGS=
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
