# $NetBSD: Makefile,v 1.1 2000/01/28 19:32:45 itojun Exp $

PROG=	pim6sd
SRCS=	mld6.c mld6_proto.c \
	inet6.c kern.c main.c config.c debug.c routesock.c vers.c callout.c \
	route.c vif.c timer.c mrt.c pim6.c pim6_proto.c rp.c crc.c trace.c \
	cfparse.y cftoken.l
CFLAGS+= -g
CPPFLAGS+=-I.
CPPFLAGS+=-DINET6 -DPIM -DIOCTL_OK_ON_RAW_SOCKET
CPPFLAGS+=-DHAVE_STDARG_H -DYY_NO_UNPUT
LDADD+=-ly -ll
YFLAGS+=-d
CLEANFILES+=	y.tab.h

MAN=	pim6stat.1 pim6sd.8 pim6sd.conf.5

afterinstall:
	install ${COPY} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} \
		${INSTALLFLAGS} ${.CURDIR}/pim6stat ${DESTDIR}${BINDIR}

.include <bsd.prog.mk>
