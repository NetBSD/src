#	$NetBSD: Makefile,v 1.8.30.1 2009/05/13 19:20:11 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

PROG=	who
SRCS=	who.c utmpentry.c
CPPFLAGS+=	-DSUPPORT_UTMPX -DSUPPORT_UTMP

.include <bsd.prog.mk>
