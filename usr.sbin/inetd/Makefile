#	from: @(#)Makefile	5.5 (Berkeley) 6/29/90
#	$Id: Makefile,v 1.8 1996/12/04 13:32:31 mrg Exp $

PROG=	inetd
MAN=	inetd.8
MLINKS=	inetd.8 inetd.conf.5

CFLAGS+=-DLIBWRAP
LDADD+= -lwrap
DPADD+= ${LIBWRAP}

.include <bsd.prog.mk>
