#	from: @(#)Makefile	5.5 (Berkeley) 6/29/90
#	$NetBSD: Makefile,v 1.9 1997/01/12 06:55:41 mikel Exp $

PROG=	inetd
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CFLAGS+=-DLIBWRAP
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
