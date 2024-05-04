#	$NetBSD: Makefile,v 1.8 2024/05/04 19:10:05 reed Exp $

.ifndef HOSTPROG
.include <bsd.own.mk>
.endif

.PATH: ${NETBSDSRCDIR}/lib/libc/citrus

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
