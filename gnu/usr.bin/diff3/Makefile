#	$NetBSD: Makefile,v 1.9 1997/10/25 10:10:32 lukem Exp $

PROG=		diff3
SRCS=		diff3.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
