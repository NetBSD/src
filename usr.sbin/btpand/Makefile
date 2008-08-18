# $NetBSD: Makefile,v 1.2 2008/08/18 08:25:32 plunky Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c sdp.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBSDP} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -lsdp -levent -lutil

.include <bsd.prog.mk>
