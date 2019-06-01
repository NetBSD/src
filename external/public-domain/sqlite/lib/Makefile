# $NetBSD: Makefile,v 1.9 2019/06/01 10:58:12 mrg Exp $

LIB=		sqlite3
INCS=		sqlite3.h sqlite3ext.h
INCSDIR=	/usr/include

SRCS=		sqlite3.c

CFLAGS+=	-DNDEBUG

FILES+=			sqlite3.pc
FILESOWN_sqlite3.pc=	${BINOWN}
FILESGRP_sqlite3.pc=	${BINGRP}
FILESMODE_sqlite3.pc=	${NONBINMODE}
FILESDIR_sqlite3.pc=	/usr/lib/pkgconfig

CLEANFILES+=sqlite3.pc

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
