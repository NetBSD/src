# $NetBSD: Makefile,v 1.5.46.1 2020/04/13 08:05:51 martin Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -levent -lutil

COPTS.tap.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
