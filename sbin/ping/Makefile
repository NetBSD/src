#	$NetBSD: Makefile,v 1.11 1999/07/02 04:53:23 itojun Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ping
MAN=	ping.8
BINOWN=	root
BINMODE=4555
LDADD=	-lm
CPPFLAGS+=	-DIPSEC
LDADD+= -lipsec
LDADD+= ${LIBIPSEC}

.include <bsd.prog.mk>
