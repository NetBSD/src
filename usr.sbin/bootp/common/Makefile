#	$NetBSD: Makefile,v 1.1 1998/03/15 01:13:25 lukem Exp $

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

NOPROFILE=	noprofile
NOPIC=		nopic

# only needed during build
libinstall::

.include <bsd.lib.mk>
