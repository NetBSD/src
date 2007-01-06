#	$NetBSD: Makefile,v 1.7 2007/01/06 19:45:29 kardel Exp $

PROG=	ntpq
MAN=	ntpq.8
SRCS=	ntpq.c ntpq-subs.c ntpq-opts.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.own.mk>

.include <bsd.prog.mk>
