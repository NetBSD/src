#	$NetBSD: Makefile,v 1.13 2002/08/19 10:17:01 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	sysctl
MAN=	sysctl.8

CPPFLAGS+=-DINET6 -DIPSEC -I${.CURDIR} -I${NETBSDSRCDIR}/sys
#CPPFLAGS+=-DTCP6

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
