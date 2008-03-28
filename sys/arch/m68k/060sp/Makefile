# $NetBSD: Makefile,v 1.10 2008/03/28 22:29:16 apb Exp $

.include <bsd.sys.mk>		# for HOST_SH, TOOL_SED

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	SED=${TOOL_SED:Q} \
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
