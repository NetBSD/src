#	$Id: Makefile,v 1.5 1994/03/09 19:07:03 jtc Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"

LDADD+=		-lgnumalloc
DPADD+=		/usr/lib/libgnumalloc.a

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
