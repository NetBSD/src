#	$NetBSD: Makefile,v 1.2 2004/07/02 00:05:23 sjg Exp $
#	from: @(#)Makefile    8.3 (Berkeley) 4/2/94

.include <bsd.own.mk>

PROG=	pwhash
SRCS=	pwhash.c pwd_gensalt.c
.PATH: ${.CURDIR}/../passwd

CPPFLAGS+=-I${.CURDIR} -DLOGIN_CAP
CPPFLAGS+= -I${.CURDIR}/../../lib/libcrypt

DPADD+= ${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.include <bsd.prog.mk>
