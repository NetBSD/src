#	$NetBSD: Makefile,v 1.13 2013/08/11 06:09:29 dholland Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
