#	$NetBSD: Makefile,v 1.14 2007/05/28 12:06:38 tls Exp $
#	from: Id: Makefile,v 1.5 1993/06/24 05:11:16 deering Exp

.include <bsd.own.mk>

USE_FORT?= yes	# network server

PROG=	mrouted
SRCS=	callout.c cfparse.y config.c igmp.c inet.c kern.c main.c prune.c \
	route.c vif.c
MAN=	mrouted.8

LDADD+=	-lutil
DPADD+=	${LIBUTIL}
CPPFLAGS+= -I${.CURDIR}

.include <bsd.prog.mk>
