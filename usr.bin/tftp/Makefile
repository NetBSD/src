#	$NetBSD: Makefile,v 1.12 2019/10/04 09:47:28 mrg Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS?=2

USE_FORT?= yes	# network client

PROG=	tftp
SRCS=	main.c tftp.c tftpsubs.c

COPTS.tftp.c+=	${${ACTIVE_CC} == "gcc" && ${HAVE_GCC:U0} == 8:? -Wno-array-bounds :}

.include <bsd.prog.mk>
