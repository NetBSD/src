#	$NetBSD: Makefile,v 1.15 2023/06/03 09:09:17 lukem Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

COPTS.printquota.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
