#	$NetBSD: Makefile,v 1.8 2011/08/25 16:22:43 joerg Exp $

.include <bsd.own.mk>

PROG=	altqd
MAN=	altqd.8 altq.conf.5

SRCS=	altqd.c libaltq2.c

CPPFLAGS+= -DALTQ -I${.CURDIR}/../libaltq

LIBALTQOBJ != cd ${.CURDIR}/../libaltq && ${PRINTOBJDIR}
DPADD+=	${LIBALTQOBJ}/libaltq.a
LDADD+=	-L${LIBALTQOBJ} -laltq

DPADD+=	${LIBUTIL} ${LIBM}
LDADD+=	-lutil -lm

COPTS.libaltq2.c+=	-Wno-format-nonliteral

.include <bsd.prog.mk>
