#	from: @(#)Makefile.inc	5.3 (Berkeley) 2/20/91
#	$Id: Makefile.inc,v 1.3 1993/08/01 05:40:32 mycroft Exp $

# compat-43 sources
.PATH: ${.CURDIR}/${MACHINE}/yp ${.CURDIR}/yp

SRCS+=	xdryp.c yplib.c

