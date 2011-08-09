#	$NetBSD: Makefile,v 1.6 2011/08/09 13:04:28 joerg Exp $

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

.include <bsd.prog.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.altqd.c+=	${${ACTIVE_CC} == "gcc":?-Wno-array-bounds:}
.endif
