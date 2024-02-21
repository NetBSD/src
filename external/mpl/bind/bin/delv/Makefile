#	$NetBSD: Makefile,v 1.3 2024/02/21 22:50:55 christos Exp $

.include <bsd.init.mk>

PROG=	delv
SRCS=	delv.c
BINDIR=	/usr/bin

DIST=${IDIST}/bin/delv
CPPFLAGS+=	-I${DIST}/include -I${IDIST}/lib/irs/include
CPPFLAGS+=	-DSYSCONFDIR=\"${SYSCONFDIR}\"
LDADD+=		-lirs -lisccfg
DPADD+=		${LIBIRS} ${LIBISCCFG}

.PATH: ${DIST} ${IDIST}/doc/man

.include <bsd.prog.mk>
