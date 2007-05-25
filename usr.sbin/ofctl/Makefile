#	from: @(#)Makefile	5.8 (Berkeley) 7/28/90
#	$NetBSD: Makefile,v 1.3 2007/05/25 18:27:25 macallan Exp $

.if ${MACHINE} == "sparc64" || ${MACHINE} == "macppc" || ${MACHINE} == "shark" \
	|| ${MACHINE} == "sparc"

PROG=	ofctl

SRCS=	ofctl.c

.endif

MAN=	ofctl.8

.include <bsd.prog.mk>
