#	$NetBSD: Makefile,v 1.1 2001/07/17 03:41:03 thorpej Exp $

MKMAN=no

.if exists(arch/${MACHINE_ARCH}/initfini_asm.S)
PROG=	initfini
SRCS=	initfini.c initfini_asm.S

WARNS?=	1

CLEANFILES+= output

.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}

regress:
	@./initfini > output
	@if ! cmp -s ${.CURDIR}/expected output; then \
		echo "FAILED"; \
	else \
		echo "PASSED"; \
	fi
.else
regress:
	@echo "FAILED -- asm stub not written for ${MACHINE_ARCH}"
.endif

.include <bsd.prog.mk>
