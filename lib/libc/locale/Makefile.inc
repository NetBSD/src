#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$Id: Makefile.inc,v 1.6 1994/05/25 01:49:32 jtc Exp $

# locale sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/locale ${.CURDIR}/locale

SRCS+=	_def_monetary.c_def_numeric.c_def_time.c \
	localeconv.c setlocale.c
