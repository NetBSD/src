#	$NetBSD: Makefile,v 1.11.10.1 2007/11/06 23:07:17 matt Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	dd
SRCS=	args.c conv.c dd.c misc.c position.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV
.else
SRCS+=		conv_tab.c
.endif

.include <bsd.prog.mk>
