#	$NetBSD: Makefile,v 1.8 1997/10/25 10:12:54 lukem Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
