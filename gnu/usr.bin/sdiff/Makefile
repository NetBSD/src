#	$Id: Makefile,v 1.4 1993/10/07 07:26:15 cgd Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"
LDADD+=		-lgnumalloc
DPADD+=		/usr/lib/libgnumalloc.a
NOMAN=		noman

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
