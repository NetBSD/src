# $NetBSD: Makefile,v 1.9 2008/03/27 07:15:16 apb Exp $

.include <bsd.sys.mk>		# for HOST_SH, TOOL_SED

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	TOOL_SED=${TOOL_SED:Q} \
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
