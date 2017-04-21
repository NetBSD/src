# $NetBSD: Makefile,v 1.6.4.1 2017/04/21 16:54:11 bouyer Exp $

WARNS=0
NOMAN=1

.include <bsd.own.mk>

PROG=	all_sync_ops_linkable

.if "${ACTIVE_CC}" == "clang"
CXXFLAGS+= -std=c++11
PROG_CXX+=	cpp_atomic_ops_linkable
.endif

proginstall:
	@echo This directory features link time only tests.

.include <bsd.prog.mk>

