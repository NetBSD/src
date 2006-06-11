#	$NetBSD: Makefile,v 1.5 2006/06/11 19:34:28 kardel Exp $

PROG=	ntpq
MAN=	ntpq.8
SRCS=	ntpq.c ntpq_ops.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign -fno-strict-aliasing
.endif

.include <bsd.prog.mk>
