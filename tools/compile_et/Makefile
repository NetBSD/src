#	$NetBSD: Makefile,v 1.5 2003/11/16 14:14:18 lukem Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}compile_et
HOST_SRCDIR=	usr.bin/compile_et
HOST_SRCS=	getarg.c print_version.c warnerr.c strupr.c get_window_size.c
HOST_CPPFLAGS=	-I${DIST}/heimdal/lib/roken

# uncommon library functions
.PATH: ${.CURDIR}/../../lib/libc/string
HOST_SRCS+=	strlcpy.c strlcat.c

make-print-version.lo: ${.CURDIR}/../../include/heimdal/version.h
make-print-version: make-print-version.lo
	${_MKTARGET_LINK}
	${HOST_LINK.c} -o ${.TARGET} ${.ALLSRC}

DPSRCS=	print_version.h
print_version.h: make-print-version
	${_MKTARGET_CREATE}
	./make-print-version print_version.h

.include "${.CURDIR}/../Makefile.host"

.PATH: ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers
CLEANFILES+=	print_version.h make-print-version make-print-version.lo
