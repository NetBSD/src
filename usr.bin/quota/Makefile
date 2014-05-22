#	$NetBSD: Makefile,v 1.10.4.2 2014/05/22 11:42:48 yamt Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

.include <bsd.prog.mk>
