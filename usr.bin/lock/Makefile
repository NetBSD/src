#	$NetBSD: Makefile,v 1.8 1997/10/11 19:16:28 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lock
BINOWN= root
BINMODE=4555
LDADD+= -lcrypt
DPADD+= ${LIBCRYPT}

.if defined(SKEY)
CPPFLAGS+=-DSKEY
LDADD+= -lcrypt -lskey
DPADD+= ${LIBCRYPT} ${LIBSKEY}
.endif

.include <bsd.prog.mk>
