#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.12 1997/04/29 16:40:13 kleink Exp $

# locale sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	localeconv.c nl_langinfo.c setlocale.c

MAN+=	nl_langinfo.3
