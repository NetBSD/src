#	$NetBSD: Makefile,v 1.6 2004/05/23 02:24:52 lukem Exp $

MKPRIVATELIB=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

.include <bsd.lib.mk>
