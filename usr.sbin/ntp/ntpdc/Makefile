# $NetBSD: Makefile,v 1.5.18.1 2007/08/21 08:40:55 ghen Exp $

PROG=	ntpdc
MAN=	ntpdc.8
SRCS=	ntpdc.c ntpdc_ops.c ntpdc-opts.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.prog.mk>
