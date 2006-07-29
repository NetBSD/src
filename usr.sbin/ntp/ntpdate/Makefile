#	$NetBSD: Makefile,v 1.5 2006/07/29 08:18:36 kardel Exp $

PROG=	ntpdate
MAN=	ntpdate.8
SRCS=	ntpdate.c

.include <bsd.own.mk>

LDADD+=		-L${LIBISC} -lisc
DPADD+=		${LIBISC}/libisc.a

LDADD+=	-lm
DPADD+=	${LIBM}

.include <bsd.prog.mk>
