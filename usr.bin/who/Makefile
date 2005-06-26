#	$NetBSD: Makefile,v 1.8 2005/06/26 17:10:28 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

WARNS=	3
PROG=	who
SRCS=	who.c utmpentry.c
CPPFLAGS+=	-DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
