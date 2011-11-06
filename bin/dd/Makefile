#	$NetBSD: Makefile,v 1.16 2011/11/06 21:22:23 jym Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

RUMPPRG=dd
SRCS=	args.c conv.c dd.c misc.c position.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV -DNO_MSGFMT -DSMALL
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
