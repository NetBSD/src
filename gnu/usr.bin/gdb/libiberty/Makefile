# $Id: Makefile,v 1.4 1997/05/31 21:21:31 cjs Exp $

LIB=		iberty
NOPROFILE=
NOPIC=

CFLAGS+=	-I$(.CURDIR)/../include
SRCS=		argv.c basename.c concat.c cplus-dem.c getopt.c getopt1.c \
		obstack.c strsignal.c ieee-float.c insque.c

# only needed during build
libinstall::

.include <bsd.lib.mk>
