#	$NetBSD: Makefile,v 1.6 2005/01/12 05:45:23 christos Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

WARNS=	3
PROG=	pwhash

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
