#	$NetBSD: Makefile,v 1.7 1997/03/24 22:06:49 christos Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
