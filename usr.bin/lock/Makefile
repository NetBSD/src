#	from: @(#)Makefile	5.5 (Berkeley) 4/16/91
#	$Id: Makefile,v 1.6 1994/06/30 03:49:43 deraadt Exp $

PROG=	lock
BINOWN= root
BINMODE=4555
CFLAGS+=-DSKEY
LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}

.include <bsd.prog.mk>
