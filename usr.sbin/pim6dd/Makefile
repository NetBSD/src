# $NetBSD: Makefile,v 1.1 1999/07/17 14:06:24 itojun Exp $

PROG=	pim6dd
SRCS=	mld6.c mld6_proto.c \
	inet6.c kern.c main.c config.c debug.c routesock.c vers.c callout.c \
	route.c vif.c timer.c mrt.c pim6.c pim6_proto.c trace.c
MAN=	pim6dd.8 pim6dd.conf.5

CPPFLAGS+=-DINET6 -DPIM -DIOCTL_OK_ON_RAW_SOCKET

.include <bsd.prog.mk>
