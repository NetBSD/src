#	$NetBSD: Makefile,v 1.3 2005/01/11 22:56:19 christos Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash
SRCS=	pwhash.c

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
CPPFLAGS+= -I${.CURDIR}/../../lib/libcrypt

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
