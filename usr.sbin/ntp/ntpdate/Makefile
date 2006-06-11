#	$NetBSD: Makefile,v 1.4 2006/06/11 19:34:28 kardel Exp $

PROG=	ntpdate
MAN=	ntpdate.8
SRCS=	ntpdate.c

.include <bsd.own.mk>

.if ${HAVE_GCC} == 4
COPTS+=	-Wno-pointer-sign -fno-strict-aliasing
.endif

LDADD+=		-L${LIBISC} -lisc
DPADD+=		${LIBISC}/libisc.a

LDADD+=	-lm
DPADD+=	${LIBM}

.include <bsd.prog.mk>
