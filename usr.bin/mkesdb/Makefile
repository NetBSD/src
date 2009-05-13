#	$NetBSD: Makefile,v 1.4.10.1 2009/05/13 19:19:58 jym Exp $

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
