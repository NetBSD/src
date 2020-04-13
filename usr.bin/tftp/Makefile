#	$NetBSD: Makefile,v 1.11.28.1 2020/04/13 08:05:48 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=2

USE_FORT?= yes	# network client

PROG=	tftp
SRCS=	main.c tftp.c tftpsubs.c

COPTS.tftp.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-array-bounds :}
COPTS.tftp.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-error=format-overflow :}

.include <bsd.prog.mk>
