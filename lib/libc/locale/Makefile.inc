#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$Id: Makefile.inc,v 1.3 1993/10/07 00:23:13 cgd Exp $

# locale sources
.PATH: ${.CURDIR}/arch/${MACHINE}/locale ${.CURDIR}/locale

SRCS+=	lconv.c localeconv.c setlocale.c
