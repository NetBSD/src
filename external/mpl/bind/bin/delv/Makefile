#	$NetBSD: Makefile,v 1.2 2020/08/03 17:23:35 christos Exp $

.include <bsd.init.mk>

PROG=	delv
SRCS=	delv.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/delv
CPPFLAGS+= -I${DIST}/include -I${IDIST}/lib/irs/include
LDADD+=    -lirs -lisccfg

delv.o: ${VERSIONFILE}

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
