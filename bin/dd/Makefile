#	$NetBSD: Makefile,v 1.13 2010/11/22 21:04:27 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

PROG=	dd
SRCS=	args.c conv.c dd.c misc.c position.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV -DSMALL
.else
SRCS+=		conv_tab.c
DPADD+= 	${LIBRUMPCLIENT}
LDADD+= 	-lrumpclient
.endif

.include <bsd.prog.mk>
