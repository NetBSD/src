#	$NetBSD: Makefile,v 1.9 2002/02/02 13:03:21 lukem Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	dd
SRCS=	args.c conv.c dd.c misc.c position.c strsuftoull.c

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV
.else
SRCS+=		conv_tab.c
.endif

WARNS=2

.include <bsd.prog.mk>
