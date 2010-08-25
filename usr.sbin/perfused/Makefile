PROG=            perfused

PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CFLAGS=        ${PERFUSE_OPT_DEBUG_FLAGS}
SRCS=           perfused.c msg.c debug.c
MAN=		perfused.8
WARNS=          4

LDADD+= -lperfuse -lpuffs #-L/usr/pkg/lib -lefence
DPADD+= ${LIBPUFFS}

.include <bsd.prog.mk>
