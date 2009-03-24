# $NetBSD: Makefile,v 1.2.10.2 2009/03/24 21:46:36 bouyer Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c sdp.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBSDP} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -lsdp -levent -lutil

.include <bsd.prog.mk>
