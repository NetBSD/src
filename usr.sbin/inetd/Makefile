#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.12 1997/10/05 16:40:24 mrg Exp $

PROG=	inetd
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CFLAGS+=-DLIBWRAP
WARNS?=	1
# Use LIBWRAP_INTERNAL for libwrap checking of inetd's `internal' services.
#CFLAGS+=-DLIBWRAP_INTERNAL
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
