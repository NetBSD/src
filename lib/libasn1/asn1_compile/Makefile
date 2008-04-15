# $NetBSD: Makefile,v 1.24 2008/04/15 21:50:22 mlelstv Exp $

NOLINT=		# defined
NOMAN=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH: ${DIST}/heimdal/lib/asn1 ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers

PROG=		asn1_compile

.if ${USETOOLS} != "yes"
LDADD+= -lutil
DPADD+= ${LIBUTIL}
.endif

WARNS?=		1

asn1_compile_SRCS = 				\
	gen.c					\
	gen_copy.c				\
	gen_decode.c				\
	gen_encode.c				\
	gen_free.c				\
	gen_glue.c				\
	gen_length.c				\
	gen_seq.c				\
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

YHEADER=	# defined

#LIBROKEN!=	cd ${NETBSDSRCDIR}/lib/libroken && ${PRINTOBJDIR}
#	 -I${LIBROKEN}				\

HOST_CPPFLAGS+= -I.				\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DKRB5=1				\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
HOST_CPPFLAGS+=-DHAVE_IPV6
.endif

DPSRCS=		print_version.h

make-print-version.lo: ${NETBSDSRCDIR}/include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

.NOPATH: print_version.h make-print-version

CLEANFILES+=	print_version.h make-print-version make-print-version.lo

# HOSTPROG will be set already if this is a src/tools build.
.ifndef HOSTPROG
HOSTPROG=	${PROG}
.include <bsd.hostprog.mk>
.endif
