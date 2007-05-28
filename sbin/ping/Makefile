#	$NetBSD: Makefile,v 1.14 2007/05/28 12:06:23 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

USE_FORT?= yes	# setuid
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
