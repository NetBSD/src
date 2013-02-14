# $NetBSD: Makefile,v 1.2 2013/02/14 19:18:38 matt Exp $

LIB=		sqlite3
INCS=		sqlite3.h sqlite3ext.h
INCSDIR=	/usr/include

SRCS=		sqlite3.c

CFLAGS+=	-DNDEBUG

LIBDPLIBS+=	m	${NETBSDSRCDIR}/lib/libm

FILES+=			sqlite3.pc
FILESOWN_sqlite3.pc=	${BINOWN}
FILESGRP_sqlite3.pc=	${BINGRP}
FILESMODE_sqlite3.pc=	${NONBINMODE}
FILESDIR_sqlite3.pc=	/usr/lib/pkgconfig

.include <bsd.lib.mk>
