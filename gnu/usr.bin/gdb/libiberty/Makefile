# $Id: Makefile,v 1.3 1997/05/30 13:43:16 cjs Exp $

LIB=		iberty
NOPROFILE=
NOPIC=

# this library is used only for the build
NOINSTALL=1

CFLAGS+=	-I$(.CURDIR)/../include
SRCS=		argv.c basename.c concat.c cplus-dem.c getopt.c getopt1.c \
		obstack.c strsignal.c ieee-float.c insque.c

.include <bsd.lib.mk>
