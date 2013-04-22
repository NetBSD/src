
PROG=	mount_pgfs
SRCS=	mount.c pgfs_subs.c pgfs_db.c pgfs_puffs.c pgfs_waitq.c pgfs_debug.c

DPADD+=	${LIBPUFFS} ${LIBUTIL}
LDADD+=	-lpuffs -lutil

NOMAN=

PG_CONFIG=	pg_config

PGINC!=		${PG_CONFIG} --includedir
PGLIBDIR!=	${PG_CONFIG} --libdir
CPPFLAGS+=	-I${PGINC}
LDFLAGS+=	-L${PGLIBDIR}
LDFLAGS+=	-Wl,-R${PGLIBDIR}
LDFLAGS+=	-lpq

DBG+=		-g
#DBG?=		-g

WARNS?=		4

.include <bsd.prog.mk>
