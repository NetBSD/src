#	$NetBSD: Makefile,v 1.8.2.1 1999/01/25 05:21:51 cgd Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	portmap
MAN=	portmap.8

CPPFLAGS+=-DLIBWRAP
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
