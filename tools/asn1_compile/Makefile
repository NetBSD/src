#	$NetBSD: Makefile,v 1.1 2001/10/25 07:45:14 thorpej Exp $

HOST_SRCDIR=	lib/libasn1/asn1_compile

DPSRCS= print_version.h
print_version.h: make-print-version.c ${.CURDIR}/../../include/heimdal/version.h
	${HOST_LINK.c} -o make-print-version ${>:M*.c}
	./make-print-version print_version.h

.include "${.CURDIR}/../Makefile.host"

.PATH: ${DIST}/heimdal/lib/roken ${DIST}/heimdal/lib/vers
CLEANFILES+=	print_version.h make-print-version make-print-version.lo

${TIMESTAMP}: Makefile
