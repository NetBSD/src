#	$NetBSD: Makefile,v 1.13 2001/02/20 00:15:56 bjh21 Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

PROG=	ping
MAN=	ping.8
BINOWN=	root
BINMODE=4555
LDADD=	-lm
DPADD=	${LIBM}
CPPFLAGS+=	-DIPSEC
LDADD+= -lipsec
DPADD+= ${LIBIPSEC}

.include <bsd.prog.mk>
