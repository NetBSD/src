#	$NetBSD: Makefile,v 1.7 2002/08/01 23:23:07 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	who
SRCS=	who.c utmpentry.c
CPPFLAGS+=	-DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
