#	$NetBSD: Makefile,v 1.1 2002/10/01 20:48:58 provos Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash
SRCS=	pwhash.c pwd_gensalt.c
.PATH: ${.CURDIR}/../passwd

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
