#	$NetBSD: Makefile,v 1.13 2003/03/07 01:06:05 fvdl Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

PROG=	mrouted
SRCS=	callout.c cfparse.y config.c igmp.c inet.c kern.c main.c prune.c \
	route.c vif.c
MAN=	mrouted.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+= -I${.CURDIR}

.include <bsd.prog.mk>
