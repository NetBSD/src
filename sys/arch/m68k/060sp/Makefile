# $NetBSD: Makefile,v 1.6.28.1 2004/08/03 10:36:30 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
