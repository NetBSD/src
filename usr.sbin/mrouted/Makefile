#	$NetBSD: Makefile,v 1.15 2011/05/26 12:56:34 joerg Exp $
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

CWARNFLAGS.clang+=	-Wno-format-extra-args

.include <bsd.prog.mk>
