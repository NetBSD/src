#	$NetBSD: Makefile,v 1.11 2005/06/26 19:10:49 christos Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	dd
SRCS=	args.c conv.c dd.c misc.c position.c

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV
.else
SRCS+=		conv_tab.c
.endif

.include <bsd.prog.mk>
