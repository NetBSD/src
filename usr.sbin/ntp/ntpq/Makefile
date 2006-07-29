#	$NetBSD: Makefile,v 1.6 2006/07/29 08:18:36 kardel Exp $

PROG=	ntpq
MAN=	ntpq.8
SRCS=	ntpq.c ntpq_ops.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.own.mk>

.include <bsd.prog.mk>
