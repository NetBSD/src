#	$Header: /cvsroot/src/sbin/init/Makefile,v 1.5 1993/04/26 14:54:38 cgd Exp $

PROG=	init
#SRCS=	init.c 
CFLAGS+= -DSECURE_CONSOLE
DPADD=	${LIBUTIL}
LDADD=	-lutil
NOMAN=noman
LDFLAGS=
.ifndef EXPORTABLE_SYSTEM
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
.endif

.include <bsd.prog.mk>
