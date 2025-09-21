# $NetBSD: Makefile,v 1.5 2025/09/21 17:54:16 thorpej Exp $

INCSDIR= /usr/include/dev/i2c
INCS=	emcfanreg.h emcfaninfo.h i2c_io.h

i2c_calls.h: ${.CURDIR}/i2c_calls
	echo "${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/i2c_calls > ${.CURDIR}/i2c_calls.h"
	${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/i2c_calls > ${.CURDIR}/i2c_calls.h

.include <bsd.kinc.mk>
