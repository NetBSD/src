#	$NetBSD: Makefile,v 1.10.4.1 2012/04/17 00:09:38 yamt Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS ?= 4
.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
