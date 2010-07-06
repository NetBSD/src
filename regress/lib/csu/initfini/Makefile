#	$NetBSD: Makefile,v 1.7 2010/07/06 05:59:56 mrg Exp $

NOMAN=		# defined

.include <bsd.own.mk>

.if exists(arch/${MACHINE_CPU}/initfini_asm.S)
PROG=		initfini
SRCS=		initfini.c initfini_asm.S
WARNS?=		1
CLEANFILES+=	output

.PATH:		${.CURDIR}/arch/${MACHINE_CPU}

regress: ${PROG}
	@./${PROG} > output
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
