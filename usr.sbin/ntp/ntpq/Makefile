#	$NetBSD: Makefile,v 1.6.6.1 2007/09/03 07:05:41 wrstuden Exp $

PROG=	ntpq
MAN=	ntpq.8
SRCS=	ntpq.c ntpq-subs.c ntpq-opts.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.own.mk>

.include <bsd.prog.mk>
