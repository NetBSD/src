# $NetBSD: Makefile,v 1.4 2010/06/04 08:33:42 jmmv Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/external/bsd/atf/dist
.PATH:		${SRCDIR}/admin ${SRCDIR}/atf-version

BINDIR=		/usr/bin
PROG_CXX=	atf-version
SRCS=		atf-version.cpp
MAN=		atf-version.1

CPPFLAGS+=	-DHAVE_CONFIG_H
CPPFLAGS+=	-I${.CURDIR}/../../lib/libatf-c
CPPFLAGS+=	-I.
LDADD+=		-latf-c++ -latf-c

WARNS?=		2

CLEANFILES=	revision.h
atf-version.cpp: revision.h
revision.h: revision-dist.h
	cp -f ${SRCDIR}/admin/revision-dist.h revision.h

.include <bsd.prog.mk>
