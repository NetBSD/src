#	$NetBSD: Makefile,v 1.12 2002/03/20 00:23:23 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	sysctl
MAN=	sysctl.8

CPPFLAGS+=-DINET6 -DIPSEC -I${.CURDIR} -I${.CURDIR}/../../sys
#CPPFLAGS+=-DTCP6

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.include <bsd.prog.mk>
