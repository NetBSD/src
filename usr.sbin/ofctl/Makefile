#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.7.12.1 2013/06/23 06:29:05 tls Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "sparc64" \
	|| ${MACHINE} == "macppc" \
	|| ${MACHINE} == "ofppc" \
	|| ${MACHINE_CPU} == "arm" \
	|| ${MACHINE} == "sparc"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
