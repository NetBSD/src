#	$NetBSD: Makefile,v 1.12 2012/02/01 17:53:01 dholland Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS ?= 4
.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
