#	$NetBSD: Makefile,v 1.15 2010/11/08 12:42:35 pooka Exp $

PROG=	raidctl 
SRCS=	rf_configure.c raidctl.c
MAN=	raidctl.8

DPADD=  ${LIBUTIL}
LDADD=  -lutil

.ifdef RUMP_ACTION
CPPFLAGS+=	-DRUMP_ACTION -DRUMP_SYS_IOCTL -DRUMP_SYS_CLOSE
LDADD+=		-lrumpclient
DBG=		-g
.endif

.include <bsd.prog.mk>
