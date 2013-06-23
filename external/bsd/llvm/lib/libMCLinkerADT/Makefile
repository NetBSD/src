#	$NetBSD: Makefile,v 1.1.6.2 2013/06/23 06:28:24 tls Exp $

LIB=	MCLinkerADT

.include <bsd.init.mk>

.PATH: ${MCLINKER_SRCDIR}/lib/ADT

SRCS=	StringEntry.cpp

.if defined(HOSTLIB)
.include <bsd.hostlib.mk>
.else
.include <bsd.lib.mk>
.endif
