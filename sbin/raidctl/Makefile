#	$NetBSD: Makefile,v 1.13 2009/10/11 12:14:05 pooka Exp $

PROG=	raidctl 
SRCS=	rf_configure.c raidctl.c
MAN=	raidctl.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.ifdef RUMP_ACTION
CPPFLAGS+=	-DRUMP_ACTION -DRUMP_SYS_IOCTL
LDADD+=		-lrumpdev_disk -lrumpdev_raidframe -lrumpdev
LDADD+=		-lrumpvfs -lrump -lrumpuser -lpthread
DBG=		-g
.endif

.include <bsd.prog.mk>
