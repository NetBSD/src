# $NetBSD: Makefile,v 1.5.8.1 2000/11/20 20:11:27 bouyer Exp $

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S: ; sh asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
