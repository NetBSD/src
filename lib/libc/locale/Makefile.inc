#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.17 2000/08/08 22:31:14 tshiozak Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	ctypeio.c localeconv.c nl_langinfo.c setlocale.c setlocale_sb.c

MAN+=	nl_langinfo.3 setlocale.3

MLINKS+=setlocale.3 localeconv.3
