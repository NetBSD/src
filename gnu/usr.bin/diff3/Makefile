#	$Id: Makefile,v 1.6 1994/03/09 19:07:00 jtc Exp $

PROG=		diff3
SRCS=		diff3.c getopt.c getopt1.c version.c
CFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		/usr/lib/libgnumalloc.a

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
