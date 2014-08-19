# $NetBSD: Makefile,v 1.7.10.1 2014/08/19 23:45:56 tls Exp $

USE_ATF_LIBTOOLS=	yes

.include <bsd.init.mk>

.PATH:		${SRCDIR}/tools

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-I${SRCDIR}/tools
CPPFLAGS+=	-I.

WARNS?=		2

DPSRCS+=	version.hpp
CLEANFILES+=	version.hpp
version.hpp: Makefile atf-version.txt
	echo "#define ATF_VERSION \"$$(cat atf-version.txt)\"" >version.hpp

.include <bsd.prog.mk>
