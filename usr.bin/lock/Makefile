#	$NetBSD: Makefile,v 1.11 2002/03/22 18:10:23 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lock
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
BINOWN= root
BINMODE=4555

.include <bsd.prog.mk>

.if (${USE_SKEY} != "no")
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif
