#	$NetBSD: Makefile,v 1.6.36.1 2009/05/13 19:20:02 jym Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
