# $NetBSD: Makefile,v 1.5 1999/02/22 21:25:05 is Exp $

# regenerate isp.S, fpsp.S, pfpsp.S

.SUFFIXES:

.SUFFIXES: .sa .S

.sa.S: ; sh asm2gas $*.sa >${.TARGET}

all: isp.S fpsp.S pfpsp.S
