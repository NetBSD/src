# $NetBSD: Makefile,v 1.2 2010/11/05 10:50:50 he Exp $

PROG=		lua
SRCS=		lua.c
BINDIR=		/usr/bin

CPPFLAGS+=	-DLUA_USE_READLINE

LDADD+=		${LIBLUA} -lm -ledit
DPADD+=		${LIBLUA}

LDADD+=		-lterminfo
DPADD+=		${LIBTERMINFO}

.include <bsd.own.mk>

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src \
		${NETBSDSRCDIR}/external/mit/lua/dist/doc

.include <bsd.prog.mk>
