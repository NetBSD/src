#	$NetBSD: Makefile,v 1.5 2005/01/12 03:35:34 christos Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
