#	$NetBSD: Makefile,v 1.17 2000/06/20 06:00:27 thorpej Exp $
#	@(#)Makefile	8.1 (Berkeley) 7/19/93

PROG=	rcp
SRCS=	rcp.c util.c

# XXX Kerberos support broken right now.

#RLOGIN= ${.CURDIR}/../../usr.bin/rlogin
#SRCS+=	krcmd.c kcmd.c
#CPPFLAGS+= -DKERBEROS -DCRYPT -I${RLOGIN}

#LDADD+= -lkrb -ldes
#DPADD+= ${LIBKRB} ${LIBDES}

#.PATH:	${RLOGIN}

.include <bsd.prog.mk>
