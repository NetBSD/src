#	$NetBSD: Makefile,v 1.4.2.2 2001/12/09 17:27:17 he Exp $

.include <bsd.own.mk>

MKMAN=no

.if (${OBJECT_FMT} == "ELF")

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

.endif

.include <bsd.prog.mk>
