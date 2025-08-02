#	$NetBSD: Makefile,v 1.3.2.1 2025/08/02 05:50:46 perseant Exp $

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
