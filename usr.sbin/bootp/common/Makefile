#	$NetBSD: Makefile,v 1.7.44.1 2017/03/20 06:58:06 pgoyette Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member

.include <bsd.lib.mk>
