# $NetBSD: Makefile,v 1.7 2003/10/26 07:25:36 lukem Exp $

.include <bsd.sys.mk>		# for HOST_SH

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S:
	${HOST_SH} asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
