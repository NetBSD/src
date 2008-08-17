# $NetBSD: Makefile,v 1.1 2008/08/17 13:20:57 plunky Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c sdp.c tap.c

CFLAGS+=-g -D_DIAGNOSTIC

DPADD+=	${LIBBLUETOOTH} ${LIBSDP} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -lsdp -levent -lutil

.include <bsd.prog.mk>
