# $NetBSD: Makefile,v 1.7.12.1 2019/06/10 22:10:05 christos Exp $

WARNS=0
NOMAN=1

.include <bsd.own.mk>

.if "${ACTIVE_CC}" == "clang"
CXXFLAGS+= -std=c++11
PROG_CXX+=	cpp_atomic_ops_linkable
.endif

proginstall:
	@echo This directory features link time only tests.

.include <bsd.prog.mk>

