#	$NetBSD: Makefile,v 1.1.14.1 2018/05/02 07:20:00 pgoyette Exp $

.include <bsd.hostinit.mk>

HOSTPROG=	makedoc
SRCS=		makedoc.c xexit.c xmalloc.c xstrdup.c

.PATH: ${IDIST}/info ${IDIST}/lib
HOST_CPPFLAGS+= -I${IDIST}/info -I${IDIST}/lib -I${.CURDIR}/../include
HOST_CPPFLAGS+= -DHOSTTOOL

.include <bsd.hostprog.mk>
