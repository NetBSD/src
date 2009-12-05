#	$NetBSD: Makefile,v 1.2 2009/12/05 10:38:27 haad Exp $
PROG=   dmt
SRCS=   dmt.c

DPADD=  ${LIBUTIL}
LDADD=  -lutil
MKMAN=  no

CFLAGS+= -g -fno-inline

LDADD+=         -lrumpdev_disk -lrumpdev_dm -lrumpdev -lprop
LDADD+=         -lrumpvfs -lrump -lrumpuser -lpthread
DBG=            -g

.include <bsd.prog.mk>
