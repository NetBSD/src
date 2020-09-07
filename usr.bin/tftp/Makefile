#	$NetBSD: Makefile,v 1.14 2020/09/07 00:26:24 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=2

USE_FORT?= yes	# network client

PROG=	tftp
SRCS=	main.c tftp.c tftpsubs.c

CWARNFLAGS.gcc+=	${GCC_NO_ADDR_OF_PACKED_MEMBER} \
			${GCC_NO_STRINGOP_OVERFLOW} \
			${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8:? -Wno-array-bounds :}

.include <bsd.prog.mk>
