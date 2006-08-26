#	$NetBSD: Makefile,v 1.6 2006/08/26 18:15:39 christos Exp $

PROG=	ntpdate
MAN=	ntpdate.8
SRCS=	ntpdate.c

.include <bsd.own.mk>

LDADD+=		-L${LIBISC} -lisc
DPADD+=		${LIBISC}/libisc.a

LDADD+=	-lutil -lm
DPADD+=	${LIBUTIL} ${LIBM}

.include <bsd.prog.mk>
