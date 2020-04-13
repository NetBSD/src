#	$NetBSD: Makefile,v 1.18.16.1 2020/04/13 07:45:05 martin Exp $
#	@(#)Makefile	8.1 (Berkeley) 5/31/93

.include <bsd.own.mk>

RUMPPRG=dd
SRCS=	args.c conv.c dd.c dd_swab.c misc.c position.c

DPADD+=	${LIBUTIL}
LDADD+=	-lutil

.ifdef SMALLPROG
CPPFLAGS+=	-DNO_CONV -DNO_MSGFMT -DNO_IOFLAG -DSMALL
.else
CPPFLAGS+=	-D_NETBSD_SOURCE
SRCS+=		conv_tab.c
.ifdef CRUNCHEDPROG
CPPFLAGS+=	-DSMALL
.endif
.endif

.include <bsd.prog.mk>
