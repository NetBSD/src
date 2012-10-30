#	$NetBSD: Makefile,v 1.15.4.2 2012/10/30 18:46:05 yamt Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

RUMPPRG=dd
SRCS=	args.c conv.c dd.c misc.c position.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV -DNO_MSGFMT -DSMALL
.else
SRCS+=		conv_tab.c
.ifdef CRUNCHEDPROG
CPPFLAGS+=	-DSMALL
.endif
.endif

.include <bsd.prog.mk>
