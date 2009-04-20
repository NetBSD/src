#	$NetBSD: Makefile,v 1.6 2009/04/20 16:05:30 drochner Exp $

NOMAN=# defined (must come before bsd.own.mk)

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

.PATH: ${NETBSDSRCDIR}/lib/libc/citrus

PROG=		mkesdb
SRCS=		yacc.y lex.l \
		citrus_bcs.c citrus_db_factory.c citrus_db_hash.c \
		citrus_lookup_factory.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc \
		-I${NETBSDSRCDIR}/lib/libc/citrus \
		-DHOSTPROG -DLIBC_SCCS -DYY_NO_INPUT
YHEADER=	1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
