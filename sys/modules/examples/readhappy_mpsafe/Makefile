#	$NetBSD: Makefile,v 1.1.2.2 2018/04/22 07:20:27 pgoyette Exp $

.include "../Makefile.inc"

#S?=	/usr/src/sys
KMOD=	readhappy_mpsafe
SRCS=	readhappy_mpsafe.c

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
