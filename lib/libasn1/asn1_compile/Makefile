# $NetBSD: Makefile,v 1.5 2000/08/05 17:41:15 garbled Exp $
DIST=		${.CURDIR}/../../../crypto/dist
.PATH: ${DIST}/heimdal/lib/asn1 ${DIST}/heimdal/lib/roken

WARNS?=  1
MKLINT= no

HOSTPROG= asn1_compile

SRCS=	parse.y lex.l main.c hash.c symbol.c gen.c			\
	gen_encode.c gen_decode.c gen_free.c gen_length.c gen_copy.c	\
	gen_glue.c							\
	getarg.c print_version.c warnerr.c strupr.c get_window_size.c   \
	strlcpy.c strlcat.c

YHEADER=	1

BINDIR=	/usr/bin

LIBROKEN!=	cd ${.CURDIR}/../../libroken && ${MAKE} print-objdir

HOST_CPPFLAGS+= -I.				\
	 -I${.CURDIR}/../../../include/heimdal	\
	 -I${LIBROKEN}				\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

NOMAN=	noman

DPSRCS=	print_version.h

make-print-version.lo: ${.CURDIR}/../../../include/heimdal/version.h

make-print-version: make-print-version.lo
	${HOST_CC} ${HOST_LDFLAGS} -o ${.TARGET} ${.ALLSRC}

print_version.h: make-print-version
	./make-print-version print_version.h

CLEANFILES+=	print_version.h make-print-version make-print-version.lo

.include <bsd.hostprog.mk>
