# $NetBSD: Makefile,v 1.6 2019/10/13 07:28:18 mrg Exp $
#

PROG=	btpand
MAN=	btpand.8
SRCS=	btpand.c bnep.c channel.c client.c packet.c server.c tap.c

DPADD+=	${LIBBLUETOOTH} ${LIBEVENT} ${LIBUTIL}
LDADD+=	-lbluetooth -levent -lutil

COPTS.tap.c+=	${GCC_NO_STRINGOP_TRUNCATION}

.include <bsd.prog.mk>
