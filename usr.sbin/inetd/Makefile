#	from: @(#)Makefile	5.5 (Berkeley) 6/29/90
#	$NetBSD: Makefile,v 1.10 1997/03/13 18:36:35 mycroft Exp $

PROG=	inetd
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CFLAGS+=-DLIBWRAP
# Use LIBWRAP_INTERNAL for libwrap checking of inetd's `internal' services.
#CFLAGS+=-DLIBWRAP_INTERNAL
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
