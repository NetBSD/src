# $NetBSD: Makefile,v 1.15 2023/06/03 09:09:08 lukem Exp $

LIB=		sqlite3
INCS=		sqlite3.h sqlite3ext.h
INCSDIR=	/usr/include

SRCS=		sqlite3.c

CFLAGS+=	-DNDEBUG

LDADD+=		-lm
DPADD+=		${LIBM}

FILES+=			sqlite3.pc
FILESOWN_sqlite3.pc=	${BINOWN}
FILESGRP_sqlite3.pc=	${BINGRP}
FILESMODE_sqlite3.pc=	${NONBINMODE}
FILESDIR_sqlite3.pc=	/usr/lib/pkgconfig

CLEANFILES+=sqlite3.pc

CWARNFLAGS.clang+=	-Wno-error=implicit-int-float-conversion
COPTS.sqlite3.c+=	${CC_WNO_RETURN_LOCAL_ADDR}

.include <bsd.own.mk>

.if ${MACHINE_CPU} == "m68k"
CWARNFLAGS+=	-Wa,--warn
.endif

.include <bsd.lib.mk>

all: sqlite3.pc
dependall: all

sqlite3.pc: ${SRCDIR}/sqlite3.h sqlite3.pc.in
	@(V=$$( (echo '#include <sqlite3.h>'; echo SQLITE_VERSION) | \
	    ${HOST_CC} -E -I${SRCDIR} - | tail -1 | tr -d '"') && \
	    ${TOOL_SED} -e s/@VERSION@/$$V/ < ${.CURDIR}/sqlite3.pc.in \
	    > ${.TARGET})
