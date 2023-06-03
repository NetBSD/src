# $NetBSD: Makefile,v 1.7 2023/06/03 09:09:20 lukem Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -levent -lutil

COPTS.tap.c+=	${CC_WNO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
