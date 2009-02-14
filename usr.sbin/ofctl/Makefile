#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.5 2009/02/14 13:56:42 abs Exp $

.if ${MACHINE_ARCH} == "sparc64" || ${MACHINE} == "macppc" \
	|| ${MACHINE} == "shark" || ${MACHINE_ARCH} == "sparc"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
