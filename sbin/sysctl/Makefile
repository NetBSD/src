#	$NetBSD: Makefile,v 1.11 2000/03/12 22:56:48 tsarna Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sysctl
MAN=	sysctl.8

CPPFLAGS+=-DINET6 -DIPSEC
#CPPFLAGS+=-DTCP6

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
