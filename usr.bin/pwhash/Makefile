#	$NetBSD: Makefile,v 1.4 2005/01/12 01:21:14 christos Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
CPPFLAGS+= -I${.CURDIR}/../../lib/libcrypt

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
