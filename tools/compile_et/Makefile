#	$NetBSD: Makefile,v 1.1 2001/09/21 23:01:08 tv Exp $

HOST_SRCDIR=	usr.bin/compile_et
HOST_SRCS=	getarg.c print_version.c warnerr.c strupr.c get_window_size.c
HOST_CPPFLAGS=	-I${DIST}/heimdal/lib/roken

# uncommon library functions
.PATH: ${.CURDIR}/../../lib/libc/string
HOST_SRCS+=	strlcpy.c strlcat.c

DPSRCS=	print_version.h
print_version.h: make-print-version.c ${.CURDIR}/../../include/heimdal/version.h
	${HOST_LINK.c} -o make-print-version ${>:M*.c}
	./make-print-version print_version.h

.include "${.CURDIR}/../Makefile.host"

.PATH: ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers
CLEANFILES+=	print_version.h make-print-version make-print-version.lo

${TIMESTAMP}: Makefile
