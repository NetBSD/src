#	$NetBSD: Makefile,v 1.13 1995/05/31 20:44:35 ragge Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/19/93

PROG=	routed
SRCS=	af.c if.c input.c main.c output.c startup.c tables.c timer.c \
	trace.c inet.c
MAN=	routed.8
#SUBDIR=	query trace
DPADD=	${LIBCOMPAT}
LDADD=	-lcompat

.include <bsd.prog.mk>
