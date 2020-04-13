#	$NetBSD: Makefile,v 1.13.26.1 2020/04/13 08:05:46 martin Exp $	
#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	quota
SRCS=	quota.c printquota.c

CPPFLAGS+= -I${NETBSDSRCDIR}/usr.bin/quota
DPADD=	${LIBQUOTA} ${LIBRPCSVC}
LDADD=	-lquota -lrpcsvc

COPTS.printquota.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
