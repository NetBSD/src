# $NetBSD: Makefile,v 1.6 2007/01/06 19:45:29 kardel Exp $

PROG=	ntpdc
MAN=	ntpdc.8
SRCS=	ntpdc.c ntpdc_ops.c ntpdc-opts.c

LDADD+=	-L${LIBISC} -lisc
LDADD+=	-ledit -ltermcap
DPADD+=	${LIBISC}/libisc.a

.include <bsd.prog.mk>
