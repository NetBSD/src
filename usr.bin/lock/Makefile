#	$NetBSD: Makefile,v 1.9.12.1 2000/10/18 04:19:23 tv Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	lock
DPADD+= ${LIBCRYPT}
LDADD+= -lcrypt
BINOWN= root
BINMODE=4555

.include <bsd.prog.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif
