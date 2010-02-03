#	$NetBSD: Makefile,v 1.6 2010/02/03 12:58:53 tsutsui Exp $

HOSTPROGNAME=	${_TOOL_PREFIX}compile_et
HOST_SRCDIR=	usr.bin/compile_et
HOST_SRCS=	getarg.c print_version.c warnerr.c strupr.c get_window_size.c
HOST_CPPFLAGS=	-I${DIST}/heimdal/lib/roken

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
