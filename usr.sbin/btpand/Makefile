# $NetBSD: Makefile,v 1.2.2.2 2008/09/18 04:30:01 wrstuden Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c sdp.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBSDP} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -lsdp -levent -lutil

.include <bsd.prog.mk>
