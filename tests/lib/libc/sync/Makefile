# $NetBSD: Makefile,v 1.5 2014/10/13 11:07:38 martin Exp $

WARNS=0
NOMAN=1

.include <bsd.own.mk>

PROG=	all_sync_ops_linkable

.if "${ACTIVE_CC}" == "clang" || \
    ("${ACTIVE_CC}" == "gcc" && "${HAVE_GCC}" == "48")

CXXFLAGS+= -std=c++11
PROG+=	cpp_atomic_ops_linkable

.endif

proginstall:
	@echo This directory features link time only tests.

.include <bsd.prog.mk>

