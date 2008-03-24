#	$NetBSD: Makefile,v 1.3.30.1 2008/03/24 07:16:34 keiichi Exp $

NOMAN=# defined (must come before bsd.own.mk)

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

.PATH: ${NETBSDSRCDIR}/lib/libc/citrus

WARNS?=	2
PROG=		mkcsmapper
SRCS=		yacc.y lex.l \
		citrus_db_factory.c citrus_bcs.c citrus_db_hash.c \
		citrus_lookup_factory.c citrus_pivot_factory.c
CPPFLAGS+=	-I. -I${.CURDIR} -I${NETBSDSRCDIR}/lib/libc \
		-I${NETBSDSRCDIR}/lib/libc/citrus \
		-I${NETBSDSRCDIR}/lib/libc/citrus/modules \
		-DHOSTPROG -DLIBC_SCCS
YHEADER=	1

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
