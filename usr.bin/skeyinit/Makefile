#	$NetBSD: Makefile,v 1.3.2.1 1997/11/08 10:38:15 lukem Exp $

PROG= skeyinit
BINOWN=root
BINMODE=4555
CPPFLAGS+= -I${.CURDIR}/../../lib/libskey
DPADD=	${LIBCRYPT} ${LIBSKEY}
LDADD=	-lcrypt -lskey

.include <bsd.prog.mk>
