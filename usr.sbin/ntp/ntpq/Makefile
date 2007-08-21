#	$NetBSD: Makefile,v 1.6.4.1 2007/08/21 08:40:56 ghen Exp $

PROG=	ntpq
MAN=	ntpq.8
SRCS=	ntpq.c ntpq-subs.c ntpq-opts.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.own.mk>

.include <bsd.prog.mk>
