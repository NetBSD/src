#	$NetBSD: Makefile,v 1.3 2001/07/17 15:24:27 simonb Exp $

.include <bsd.own.mk>

MKMAN=no

.if exists(arch/${MACHINE_CPU}/initfini_asm.S)
PROG=	initfini
SRCS=	initfini.c initfini_asm.S

WARNS?=	1

CLEANFILES+= output

.PATH: ${.CURDIR}/arch/${MACHINE_CPU}

regress:
	@./initfini > output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi
.else
regress:
	@echo "FAILED -- asm stub not written for ${MACHINE_CPU}"
.endif

.include <bsd.prog.mk>
