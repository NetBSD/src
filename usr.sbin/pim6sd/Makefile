# $NetBSD: Makefile,v 1.4 2000/07/04 20:27:38 matt Exp $

PROG=	pim6sd
SRCS=	mld6.c mld6_proto.c \
	inet6.c kern.c main.c config.c debug.c routesock.c vers.c callout.c \
	route.c vif.c timer.c mrt.c pim6.c pim6_proto.c rp.c crc.c trace.c \
	cfparse.y cftoken.l
CFLAGS+= -g
CPPFLAGS+=-I. -I${.CURDIR}
CPPFLAGS+=-DINET6 -DPIM -DIOCTL_OK_ON_RAW_SOCKET
CPPFLAGS+=-DHAVE_STDARG_H -DYY_NO_UNPUT -DHAVE_GETIFADDRS
LDADD+=-ly -ll
YFLAGS+=-d
CLEANFILES+=	y.tab.h
SCRIPTS=pim6stat

MAN=	pim6stat.1 pim6sd.8 pim6sd.conf.5

.include <bsd.prog.mk>
