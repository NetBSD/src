#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$Id: Makefile.inc,v 1.9 1994/07/16 05:12:26 jtc Exp $

# locale sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/locale ${.CURDIR}/locale

SRCS+=	_def_monetary.c _def_numeric.c _def_time.c \
	localeconv.c nl_langinfo.c setlocale.c

MAN3+=	nl_langinfo.0
