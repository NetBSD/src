#	$NetBSD: Makefile,v 1.4 1997/10/24 09:01:01 lukem Exp $

PROG= skeyinit
BINOWN=root
BINMODE=4555
CPPFLAGS+= -I${.CURDIR}/../../lib/libskey
DPADD=	${LIBCRYPT} ${LIBSKEY}
LDADD=	-lcrypt -lskey

.include <bsd.prog.mk>
