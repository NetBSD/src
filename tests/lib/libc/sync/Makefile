# $NetBSD: Makefile,v 1.4 2014/10/13 09:57:35 martin Exp $

WARNS=0
NOMAN=1

proginstall:
	@echo This directory features link time only tests.

.include <bsd.prog.mk>

PROG=	all_sync_ops_linkable

.if "${ACTIVE_CC}" == "clang" || \
    ("${ACTIVE_CC}" == "gcc" && "${HAVE_GCC}" == "48")

CXXFLAGS+= -std=c++11
PROG+=	cpp_atomic_ops_linkable

.endif

