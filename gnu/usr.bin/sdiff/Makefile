#	$Id: Makefile,v 1.3 1993/09/16 17:40:07 jtc Exp $

PROG=		sdiff
SRCS=		sdiff.c getopt.c getopt1.c version.c
CFLAGS+=	-I$(.CURDIR)/../diff \
		-DHAVE_CONFIG_H -DDIFF_PROGRAM=\"/usr/bin/diff\"
NOMAN=		noman

.include <bsd.prog.mk>
.PATH: $(.CURDIR)/../diff
