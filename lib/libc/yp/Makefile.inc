#	$NetBSD: Makefile.inc,v 1.8 1995/02/27 13:00:40 cgd Exp $

# yp sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/yp ${.CURDIR}/yp

SRCS+=	xdryp.c yplib.c
MAN+=	ypclnt.3
