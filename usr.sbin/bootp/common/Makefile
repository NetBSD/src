#	$NetBSD: Makefile,v 1.6.24.1 2008/05/18 12:36:14 yamt Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

.include <bsd.lib.mk>
