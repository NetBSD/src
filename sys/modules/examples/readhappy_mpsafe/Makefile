#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:49 rin Exp $

.include "../Makefile.inc"

#S?=	/usr/src/sys
KMOD=	readhappy_mpsafe
SRCS=	readhappy_mpsafe.c

WARNS=	3

.include <bsd.kmodule.mk>

# To make use of this module, you'll need to separately build the
# test_readhappy program, with a Makefile similar to
#
#	MKMAN=	NO
#	PROG=	test_readhappy
#
#	CFLAGS+=-pthread
#
#	LDADD+= -lpthread
#	DPADD+= ${LIBPTHREAD}
#
#	.include <bsd.prog.mk>
