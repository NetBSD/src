#	$NetBSD: Makefile,v 1.9 1999/01/11 20:51:09 jwise Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	portmap
MAN=	portmap.8

CPPFLAGS+=-DLIBWRAP
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
