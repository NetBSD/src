#	$NetBSD: Makefile,v 1.2 2003/08/01 17:04:03 lukem Exp $

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
		-DHOSTPROG
YHEADER=	1
MKMAN=		no

.ifndef HOSTPROG
.include <bsd.prog.mk>
.endif
