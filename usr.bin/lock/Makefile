#	$NetBSD: Makefile,v 1.12 2007/05/28 12:06:27 tls Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

USE_FORT?= yes	# setuid
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
