#	from: @(#)Makefile	5.5 (Berkeley) 5/11/90
#	$Id: Makefile,v 1.6 1994/05/24 06:52:17 deraadt Exp $

PROG=	su
BINOWN=	root
BINMODE=4555
CFLAGS+=-DSKEY
LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}

.include <bsd.prog.mk>
