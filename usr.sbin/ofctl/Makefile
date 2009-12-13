#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.6 2009/12/13 05:01:33 nakayama Exp $

.if ${MACHINE} == "sparc64" || ${MACHINE} == "macppc" \
	|| ${MACHINE} == "shark" || ${MACHINE} == "sparc"

PROG=	ofctl

SRCS=	ofctl.c

LDADD+=	-lprop
DPADD+=	${LIBPROP}

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
