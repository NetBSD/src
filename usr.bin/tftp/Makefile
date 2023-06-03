#	$NetBSD: Makefile,v 1.16 2023/06/03 09:09:19 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=2

USE_FORT?= yes	# network client

PROG=	tftp
SRCS=	main.c tftp.c tftpsubs.c

CWARNFLAGS.gcc+=	${CC_WNO_STRINGOP_OVERFLOW} \
			${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} >= 8:? -Wno-array-bounds :}

.include <bsd.prog.mk>
