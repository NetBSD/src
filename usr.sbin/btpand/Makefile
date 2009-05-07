# $NetBSD: Makefile,v 1.3 2009/05/07 18:08:55 plunky Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c sdp.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBSDP} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -lsdp -levent -lutil

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
