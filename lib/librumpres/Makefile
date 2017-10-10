# $NetBSD: Makefile,v 1.1 2017/10/10 19:28:33 christos Exp $

USE_FORT?=	yes
USE_SHLIBDIR=   yes

.include <bsd.own.mk>

LIB=	rumpres

SHLIB_MAJOR=0
SHLIB_MINOR=0

LIBDPLIBS+=     rumpclient    ${.CURDIR}/../librumpclient

.if ${USE_INET6} != "no"
CPPFLAGS+=      -DINET6
.endif

.PATH:		${.CURDIR}/../libc/net
SRCS=		getaddrinfo.c getifaddrs.c getnameinfo.c
SRCS+=		if_indextoname.c if_nametoindex.c
#SRCS+=		gethnamaddr.c sethostent.c
CPPFLAGS+= 	-DRUMP_ACTION

.include <bsd.lib.mk>
