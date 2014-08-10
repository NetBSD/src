# $NetBSD: Makefile,v 1.3.22.1 2014/08/10 06:50:54 tls Exp $

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
