# $NetBSD: Makefile,v 1.11 2001/10/25 07:42:39 thorpej Exp $

NOMAN=	noman

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../../crypto/dist
.PATH: ${DIST}/heimdal/lib/asn1 ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers

WARNS?=  1
MKLINT= no

.ifdef HOSTPROG
# This is is what src/tools/Makefile.host expects
PROG=	asn1_compile
XXX_CROSS_TOOL=yes	# temporary hack
.else
HOSTPROG= asn1_compile
.endif

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

#LIBROKEN!=	cd ${.CURDIR}/../../libroken && ${PRINTOBJDIR}
#	 -I${LIBROKEN}				\

HOST_CPPFLAGS+= -I.				\
	 -I${.CURDIR}/../../../include/heimdal	\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.ifndef XXX_CROSS_TOOL
DPSRCS=	print_version.h

make-print-version.lo: ${.CURDIR}/../../../include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo

.include <bsd.hostprog.mk>
.endif
