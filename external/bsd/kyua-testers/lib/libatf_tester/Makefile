# $NetBSD: Makefile,v 1.1.12.2 2014/05/22 15:45:39 yamt Exp $

#NOLINT=		# defined

.include <bsd.own.mk>

LIB=		atf_tester
LIBISPRIVATE=	yes

SRCDIR=		${NETBSDSRCDIR}/external/bsd/kyua-testers/dist
.PATH:		${SRCDIR}

CPPFLAGS+=	-I${.CURDIR}/../libtester  # For config.h and defs.h.
CPPFLAGS+=	-I${SRCDIR}  # For non-installed headers.

CPPFLAGS+=	-DHAVE_CONFIG_H

WARNS?=		5

SRCS=		atf_list.c
SRCS+=		atf_result.c

.include <bsd.lib.mk>
