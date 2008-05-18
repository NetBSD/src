# $NetBSD: Makefile,v 1.1.8.1 2008/05/18 12:36:04 yamt Exp $

.include <bsd.own.mk>

SRCDIR=		${NETBSDSRCDIR}/dist/atf
.PATH:		${SRCDIR}/admin ${SRCDIR}/tools

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
