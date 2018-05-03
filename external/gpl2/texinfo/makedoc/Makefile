#	$NetBSD: Makefile,v 1.3 2018/05/03 01:12:26 nonaka Exp $

MAKEDOC=
.include <bsd.hostinit.mk>

HOSTPROG=	makedoc
SRCS=		makedoc.c xexit.c xmalloc.c xstrdup.c

.PATH: ${IDIST}/info ${IDIST}/lib
HOST_CPPFLAGS+= -I${IDIST}/info -I${IDIST}/lib -I${.CURDIR}/../include
HOST_CPPFLAGS+= -DHOSTTOOL

.include <bsd.hostprog.mk>
