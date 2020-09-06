#	$NetBSD: Makefile,v 1.9 2020/09/06 07:20:31 mrg Exp $

LIBISPRIVATE=	yes

LIB=	common
SRCS=	dovend.c dumptab.c getif.c hash.c hwaddr.c lookup.c readfile.c \
	report.c tzone.c
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG

CWARNFLAGS.clang+=	-Wno-error=address-of-packed-member
CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER}

.include <bsd.lib.mk>
