# $NetBSD: Makefile,v 1.5.20.1 2007/09/03 07:05:40 wrstuden Exp $

PROG=	ntpdc
MAN=	ntpdc.8
SRCS=	ntpdc.c ntpdc_ops.c ntpdc-opts.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.prog.mk>
