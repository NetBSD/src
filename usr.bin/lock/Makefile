#	$NetBSD: Makefile,v 1.9 1997/10/12 12:56:51 mycroft Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lock
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
BINOWN= root
BINMODE=4555
INSTALLFLAGS=-fschg

.include <bsd.prog.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif
