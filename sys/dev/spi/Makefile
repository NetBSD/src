# $NetBSD: Makefile,v 1.2 2025/09/14 00:28:44 thorpej Exp $

INCSDIR= /usr/include/dev/spi
INCS=   spi_io.h

spi_calls.h: ${.CURDIR}/spi_calls
	echo "${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/spi_calls > ${.CURDIR}/spi_calls.h"
	${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/spi_calls > ${.CURDIR}/spi_calls.h

.include <bsd.kinc.mk>
