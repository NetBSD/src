#	$NetBSD: Makefile,v 1.7.2.1 1997/11/08 06:54:18 lukem Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CPPFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		${LIBGNUMALLOC}

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
