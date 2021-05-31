#	$NetBSD: Makefile,v 1.9.2.1 2021/05/31 22:15:26 cjep Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.lib.mk>
