#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.12 2026/07/19 01:48:19 thorpej Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" \
	|| ${MACHINE} == "ofppc" \
	|| ${MACHINE_CPU} == "arm" \
	|| ${MACHINE_CPU} == "aarch64" \
	|| ${MACHINE} == "sparc" \
	|| ${MACHINE} == "evbmips" \
	|| ${MACHINE_CPU} == "riscv" \
	|| ${MACHINE} == "hb68k"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
