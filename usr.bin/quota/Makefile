#	$NetBSD: Makefile,v 1.11 2012/01/09 15:35:44 dholland Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS ?= 4
.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC} ${LIBPROP}
LDADD=	-lquota -lrpcsvc -lprop

.include <bsd.prog.mk>
