#	$NetBSD: Makefile,v 1.5 2002/09/18 14:00:40 lukem Exp $

.include <bsd.own.mk>

PROG= skeyinit
BINOWN=root
BINMODE=4555
CPPFLAGS+= -I${NETBSDSRCDIR}/lib/libskey
DPADD=	${LIBCRYPT} ${LIBSKEY}
LDADD=	-lcrypt -lskey

.include <bsd.prog.mk>
