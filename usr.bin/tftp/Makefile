#	$NetBSD: Makefile,v 1.13 2020/02/08 13:33:56 fox Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=2

USE_FORT?= yes	# network client

PROG=	tftp
SRCS=	main.c tftp.c tftpsubs.c

COPTS.tftp.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-array-bounds :}
COPTS.tftp.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-error=format-overflow :}

.include <bsd.prog.mk>
