# $NetBSD: Makefile,v 1.7 2017/02/22 12:34:49 maya Exp $

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

