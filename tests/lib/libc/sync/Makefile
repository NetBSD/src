# $NetBSD: Makefile,v 1.8 2019/02/26 10:01:41 isaki Exp $

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

