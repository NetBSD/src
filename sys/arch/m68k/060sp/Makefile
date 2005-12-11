# $NetBSD: Makefile,v 1.8 2005/12/11 12:17:52 christos Exp $

.include <bsd.sys.mk>		# for HOST_SH

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
