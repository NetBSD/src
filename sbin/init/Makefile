#	$Header: /cvsroot/src/sbin/init/Makefile,v 1.4 1993/03/23 08:17:35 cgd Exp $

PROG=	init
#SRCS=	init.c 
CFLAGS+= -DSECURE_CONSOLE -DDES
DPADD=	${LIBUTIL}
LDADD=	-lutil
NOMAN=noman
LDFLAGS=

.include <bsd.prog.mk>
