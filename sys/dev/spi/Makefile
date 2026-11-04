# $NetBSD: Makefile,v 1.1.38.1 2026/11/04 23:22:34 perseant Exp $

INCSDIR= /usr/include/dev/spi
INCS=   spi_io.h

spi_calls.h: ${.CURDIR}/spi_calls
	echo "${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/spi_calls > ${.CURDIR}/spi_calls.h"
	${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/spi_calls > ${.CURDIR}/spi_calls.h

.include <bsd.kinc.mk>
