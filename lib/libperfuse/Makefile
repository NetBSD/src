LIB=            perfuse
LIBDPLIBS+=     puffs	/usr/src/lib/libpuffs


PERFUSE_OPT_DEBUG_FLAGS=   -g -DPERFUSE_DEBUG

CFLAGS+=        ${PERFUSE_OPT_DEBUG_FLAGS}
SRCS=           perfuse.c ops.c subr.c debug.c
MAN=		libperfuse.3
WARNS=          4
INCS=           perfuse.h
INCSDIR=        /usr/include

.include <bsd.lib.mk>
