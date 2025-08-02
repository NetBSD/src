#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.10.8.1 2025/08/02 05:58:53 perseant Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" \
	|| ${MACHINE} == "ofppc" \
	|| ${MACHINE_CPU} == "arm" \
	|| ${MACHINE_CPU} == "aarch64" \
	|| ${MACHINE} == "sparc" \
	|| ${MACHINE} == "evbmips" \
	|| ${MACHINE_CPU} == "riscv"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
