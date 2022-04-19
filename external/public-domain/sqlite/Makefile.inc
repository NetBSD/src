# $NetBSD: Makefile.inc,v 1.12 2022/04/19 22:40:45 rillig Exp $

WARNS=		2

SRCDIR:=	${.PARSEDIR}/dist

.PATH: ${SRCDIR}

CPPFLAGS+=	\
		-DHAVE_GMTIME_R=1 \
		-DHAVE_INTTYPES_H=1 \
		-DHAVE_LOCALTIME_R=1 \
		-DHAVE_READLINE=1 \
		-DHAVE_STDINT_H=1 \
		-DHAVE_STRERROR_R=1 \
		-DHAVE_USLEEP=1 \
		-DHAVE_SYS_ENDIAN_H=1 \
		-DSQLITE_THREADSAFE \
		-DSQLITE_MAX_WORKER_THREADS=0 \
		-DSQLITE_ENABLE_COLUMN_METADATA \
		-DSQLITE_ENABLE_FTS3_PARENTHESIS \
		-DSQLITE_ENABLE_FTS4 \
		-DSQLITE_ENABLE_FTS5 \
		-DSQLITE_ENABLE_JSON1 \
		-DSQLITE_ENABLE_RTREE \
		-DSQLITE_ENABLE_GEOPOLY \
		-DSQLITE_ENABLE_LOAD_EXTENSION \
		-DSQLITE_ENABLE_UNLOCK_NOTIFY \
		-DSQLITE_HAVE_ISNAN

CWARNFLAGS+=	-Wno-shadow -Wno-unused

LINTFLAGS+=	-X 117	# bitwise '>>' on signed value possibly nonportable
LINTFLAGS+=	-X 161	# constant in conditional context
LINTFLAGS+=	-X 220	# fallthrough on case statement
LINTFLAGS+=	-X 247	# pointer cast from '%s' to '%s' may be troublesome
LINTFLAGS+=	-X 275	# cast discards 'const' from type '%s'
