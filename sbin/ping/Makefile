#	$NetBSD: Makefile,v 1.12 1999/07/12 19:00:52 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ping
MAN=	ping.8
BINOWN=	root
BINMODE=4555
LDADD=	-lm
CPPFLAGS+=	-DIPSEC
LDADD+= -lipsec
DPADD+= ${LIBIPSEC}

.include <bsd.prog.mk>
