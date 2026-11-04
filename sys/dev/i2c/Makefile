# $NetBSD: Makefile,v 1.3.92.2 2026/11/04 23:21:54 perseant Exp $

INCSDIR= /usr/include/dev/i2c
INCS=	emcfanreg.h emcfaninfo.h i2c_io.h

i2c_calls.h: ${.CURDIR}/i2c_calls
	echo "${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/i2c_calls > ${.CURDIR}/i2c_calls.h"
	${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/i2c_calls > ${.CURDIR}/i2c_calls.h

.include <bsd.kinc.mk>
