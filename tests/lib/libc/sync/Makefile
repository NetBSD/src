# $NetBSD: Makefile,v 1.6.2.1 2017/03/20 06:57:59 pgoyette Exp $

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

