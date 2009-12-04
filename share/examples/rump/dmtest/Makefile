#	$NetBSD: Makefile,v 1.1 2009/12/04 22:18:30 haad Exp $
PROG=   dmt
SRCS=   dmt.c

DPADD=  ${LIBUTIL}
LDADD=  -lutil
MKMAN=  no

CFLAGS+= -g -fno-inline

CPPFLAGS+=      -DRUMP_ACTION -DRUMP_SYS_IOCTL -DRUMP_SYS_CLOSE -I${NETBSDSRCDIR}/sys/dev/dm
LDADD+=         -lrumpdev_disk -lrumpdev_dm -lrumpdev -lprop
LDADD+=         -lrumpvfs -lrump -lrumpuser -lpthread
DBG=            -g

.include <bsd.prog.mk>
