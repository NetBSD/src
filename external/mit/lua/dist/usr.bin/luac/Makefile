# $NetBSD: Makefile,v 1.1.1.1 2018/07/29 19:46:04 alnsn Exp $

PROG=		luac
SRCS=		luac.c
BINDIR=		/usr/bin

#
# statically link ${LIBLUA} as "hidden" symbols need to be accessed
#
DPADD+=		${LIBLUA} ${LIBM}
LDADD+=		${LIBLUA} -lm

.include <bsd.own.mk>

.PATH:		${NETBSDSRCDIR}/external/mit/lua/dist/src \
		${NETBSDSRCDIR}/external/mit/lua/dist/doc

.include <bsd.prog.mk>
