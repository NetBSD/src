# $NetBSD: Makefile,v 1.9 2001/09/17 12:34:41 assar Exp $

NOMAN=	noman

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../crypto/dist
.PATH: ${DIST}/heimdal/lib/asn1 ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers

WARNS?=  1
MKLINT= no

HOSTPROG= asn1_compile

asn1_compile_SRCS = 				\
	gen.c					\
	gen_copy.c				\
	gen_decode.c				\
	gen_encode.c				\
	gen_free.c				\
	gen_glue.c				\
	gen_length.c				\
	hash.c					\
	lex.l					\
	main.c					\
	parse.y					\
	symbol.c

roken_SRCS =					\
	getarg.c				\
	print_version.c				\
	warnerr.c				\
	strupr.c				\
	get_window_size.c

SRCS=	$(asn1_compile_SRCS)			\
	$(roken_SRCS)

YHEADER=	1

BINDIR=	/usr/bin

LIBROKEN!=	cd ${.CURDIR}/../../libroken && ${PRINTOBJDIR}

HOST_CPPFLAGS+= -I.				\
	 -I${.CURDIR}/../../../include/heimdal	\
	 -I${LIBROKEN}				\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

DPSRCS=	print_version.h

make-print-version.lo: ${.CURDIR}/../../../include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo

.include <bsd.hostprog.mk>
