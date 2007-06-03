#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.4 2007/06/03 03:01:41 matt Exp $

.if ${MACHINE} == "sparc64" || ${MACHINE} == "macppc" || ${MACHINE} == "shark" \
	|| ${MACHINE} == "sparc"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
