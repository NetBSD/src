#	$NetBSD: Makefile,v 1.5 2011/06/22 05:23:54 mrg Exp $

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
COPTS.altqd.c+=	-Wno-array-bounds
.endif
