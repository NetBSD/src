#	$NetBSD: Makefile,v 1.10 2000/07/04 20:27:38 matt Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

PROG=	mrouted
SRCS=	callout.c cfparse.y config.c igmp.c inet.c kern.c main.c prune.c \
	route.c vif.c
MAN=	mrouted.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+=-I${.CURDIR}

.include <bsd.prog.mk>
