#	from: @(#)Makefile	5.5 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.7 1996/11/26 17:23:34 mrg Exp $

PROG=	inetd
MAN=	inetd.8
MLINKS=	inetd.8 inetd.5

CFLAGS+=-DLIBWRAP
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
