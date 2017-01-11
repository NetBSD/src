#	$NetBSD: Makefile,v 1.8 2017/01/11 12:19:44 joerg Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

.include <bsd.lib.mk>
