# $NetBSD: Makefile,v 1.10.48.1 2014/08/20 00:03:10 tls Exp $

.include <bsd.own.mk>		# for HOST_SH, TOOL_SED

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	SED=${TOOL_SED:Q} \
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}


all:	isp.S fpsp.S pfpsp.S
CLEANFILES+=isp.S fpsp.S pfpsp.S

.include <bsd.files.mk>
