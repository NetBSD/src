#	$NetBSD: Makefile,v 1.6 2003/12/23 19:29:11 junyoung Exp $

NOMAN=		# defined

.include <bsd.own.mk>

.if (${OBJECT_FMT} == "ELF")

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

.endif

.include <bsd.prog.mk>
