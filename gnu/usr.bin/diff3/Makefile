#	$NetBSD: Makefile,v 1.8.2.1 1997/11/08 06:52:54 lukem Exp $

PROG=		diff3
SRCS=		diff3.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
