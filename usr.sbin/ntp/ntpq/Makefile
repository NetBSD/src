#	$NetBSD: Makefile,v 1.4 2003/12/04 16:41:57 drochner Exp $

PROG=	ntpq
MAN=	ntpq.8
SRCS=	ntpq.c ntpq_ops.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.prog.mk>
