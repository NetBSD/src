# $NetBSD: Makefile,v 1.3 2011/01/19 10:17:04 plunky Exp $

PROG=		lua
SRCS=		lua.c
BINDIR=		/usr/bin

CPPFLAGS+=	-DLUA_USE_READLINE

LDADD+=		${LIBLUA}
DPADD+=		${LIBLUA}

LDADD+=		-lterminfo -lm -ledit
DPADD+=		${LIBTERMINFO} ${LIBM} ${LIBEDIT}

.include <bsd.own.mk>

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src \
		${NETBSDSRCDIR}/external/mit/lua/dist/doc

.include <bsd.prog.mk>
