#	$Id: Makefile,v 1.4 1993/09/16 17:39:59 jtc Exp $

PROG=		diff3
SRCS=		diff3.c getopt.c getopt1.c version.c
CFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"
NOMAN=		noman

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
