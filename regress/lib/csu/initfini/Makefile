#	$NetBSD: Makefile,v 1.2 2001/07/17 05:40:28 thorpej Exp $

.include <bsd.own.mk>

MKMAN=no

.if exists(arch/${MACHINE_CPU}/initfini_asm.S)
PROG=	initfini
SRCS=	initfini.c initfini_asm.S

WARNS?=	1

CLEANFILES+= output

.PATH: ${.CURDIR}/arch/${MACHINE_CPU}

.if (${MACHINE_CPU} == "mips")
AFLAGS+= -fPIC
.endif

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
