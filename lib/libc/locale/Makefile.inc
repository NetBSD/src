#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$Id: Makefile.inc,v 1.7 1994/05/26 00:31:53 deraadt Exp $

# locale sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/locale ${.CURDIR}/locale

SRCS+=	_def_monetary.c _def_numeric.c _def_time.c \
	localeconv.c setlocale.c
