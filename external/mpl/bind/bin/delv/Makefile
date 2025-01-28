#	$NetBSD: Makefile,v 1.5 2025/01/28 15:00:43 christos Exp $

.include <bsd.init.mk>

PROG=	delv
SRCS=	delv.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/delv
CPPFLAGS+=	-I${DIST}/include -I${IDIST}/lib/irs/include
CPPFLAGS+=	-DSYSCONFDIR=\"${SYSCONFDIR}\"
LDADD+=		-ldns -lisccfg
DPADD+=		${LIBDNS} ${LIBISCCFG}

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
