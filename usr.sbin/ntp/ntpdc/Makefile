# $NetBSD: Makefile,v 1.5 2003/12/04 16:41:57 drochner Exp $

PROG=	ntpdc
MAN=	ntpdc.8
SRCS=	ntpdc.c ntpdc_ops.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.prog.mk>
