#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$Id: Makefile.inc,v 1.10 1994/09/29 04:57:23 jtc Exp $

# locale sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	localeconv.c nl_langinfo.c setlocale.c

MAN3+=	nl_langinfo.0
