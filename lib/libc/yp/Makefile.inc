#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.7 1994/12/22 10:15:18 cgd Exp $

# yp sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/yp ${.CURDIR}/yp

SRCS+=	xdryp.c yplib.c
MAN+=	ypclnt.3
