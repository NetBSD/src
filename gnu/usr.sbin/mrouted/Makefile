#	$NetBSD: Makefile,v 1.2 2002/10/17 12:50:00 itojun Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

PROG=	mrouted
SRCS=	callout.c cfparse.y config.c igmp.c inet.c kern.c main.c prune.c \
	route.c vif.c
MAN=	mrouted.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+=-I${NETBSDSRCDIR}/gnu/dist/mrouted -I${.CURDIR}

.PATH:	${NETBSDSRCDIR}/gnu/dist/mrouted

.include <bsd.prog.mk>
