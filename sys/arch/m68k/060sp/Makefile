# $NetBSD: Makefile,v 1.6.28.3 2004/09/21 13:17:25 skrll Exp $

.include <bsd.sys.mk>		# for HOST_SH

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
