# $NetBSD: Makefile,v 1.10 2001/09/21 22:58:59 tv Exp $

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
	$(roken_SRCS)				\
	nb_progname.c

YHEADER=	1

#LIBROKEN!=	cd ${.CURDIR}/../../libroken && ${PRINTOBJDIR}
#	 -I${LIBROKEN}				\

HOST_CPPFLAGS+= -I.				\
	 -I${.CURDIR}/../../../include/heimdal	\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

DPSRCS=	print_version.h

print_version.h: make-print-version.c ${.CURDIR}/../../../include/heimdal/version.h
	${HOST_LINK.c} -o make-print-version ${>:M*.c}
	./make-print-version print_version.h

CLEANFILES+=	print_version.h make-print-version

.include <bsd.hostprog.mk>
