#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.10 2020/07/24 12:07:09 jmcneill Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" \
	|| ${MACHINE} == "ofppc" \
	|| ${MACHINE_CPU} == "arm" \
	|| ${MACHINE_CPU} == "aarch64" \
	|| ${MACHINE} == "sparc" \
	|| ${MACHINE} == "evbmips"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
