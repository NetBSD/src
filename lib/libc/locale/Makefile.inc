#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.19 2000/12/21 05:12:19 itojun Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	ctypeio.c localeconv.c nl_langinfo.c setlocale.c setlocale_sb.c \
	wcscat.c wcschr.c wcscmp.c wcscpy.c wcscspn.c wcslen.c wcsncat.c \
	wcsncmp.c wcsncpy.c wcspbrk.c wcsrchr.c wcsspn.c wcsstr.c \
	wmemchr.c wmemcmp.c wmemcpy.c wmemmove.c wmemset.c
MAN+=	nl_langinfo.3 setlocale.3

MLINKS+=setlocale.3 localeconv.3
