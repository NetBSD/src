# $NetBSD: Makefile,v 1.2.8.1 2009/05/13 19:20:19 jym Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -levent -lutil

CPPFLAGS+=	-DSDP_COMPAT

.include <bsd.prog.mk>
