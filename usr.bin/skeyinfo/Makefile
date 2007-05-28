#	$NetBSD: Makefile,v 1.2 2007/05/28 12:06:31 tls Exp $

USE_FORT?= yes	# setuid
PROG= skeyinfo
MAN= skeyinfo.1
DPADD=	${LIBSKEY}
LDADD=	-lskey
BINOWN= root
BINMODE=4555

.include <bsd.prog.mk>
