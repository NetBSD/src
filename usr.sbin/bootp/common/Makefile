#	$NetBSD: Makefile,v 1.7 2008/05/03 14:48:31 lukem Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

.include <bsd.lib.mk>
