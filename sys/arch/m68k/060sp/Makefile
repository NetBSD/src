# $NetBSD: Makefile,v 1.6 2000/04/14 20:24:29 is Exp $

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.PATH: dist

.sa.S: ; sh asm2gas ${.IMPSRC} >${.TARGET}

all: isp.S fpsp.S pfpsp.S
