#	$NetBSD: Makefile,v 1.2 1999/02/13 02:54:58 lukem Exp $

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

MKPROFILE=	no
MKPIC=		no

# only needed during build
libinstall::

.include <bsd.lib.mk>
