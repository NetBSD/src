#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.5 1993/12/04 02:40:06 cgd Exp $

# yp sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/yp ${.CURDIR}/yp

SRCS+=	xdryp.c yplib.c

