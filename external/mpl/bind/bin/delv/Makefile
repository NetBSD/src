#	$NetBSD: Makefile,v 1.2.6.1 2024/02/25 15:42:52 martin Exp $

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
