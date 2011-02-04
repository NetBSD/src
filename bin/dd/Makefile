#	$NetBSD: Makefile,v 1.15 2011/02/04 19:42:12 pooka Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

RUMPPRG=dd
SRCS=	args.c conv.c dd.c misc.c position.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV -DSMALL
.else
SRCS+=		conv_tab.c
.ifndef CRUNCHEDPROG
DPADD+= 	${LIBRUMPCLIENT}
LDADD+= 	-lrumpclient
.else
CPPFLAGS+=	-DSMALL
.endif
.endif

.include <bsd.prog.mk>
