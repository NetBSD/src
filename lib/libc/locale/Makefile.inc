#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.23 2000/12/21 12:13:28 itojun Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	ctypeio.c iswctype.c localeconv.c nl_langinfo.c \
	setlocale.c setlocale_sb.c \
	wcscat.c wcschr.c wcscmp.c wcscpy.c wcscspn.c wcslen.c wcsncat.c \
	wcsncmp.c wcsncpy.c wcspbrk.c wcsrchr.c wcsspn.c wcsstr.c wcswidth.c \
	wmemchr.c wmemcmp.c wmemcpy.c wmemmove.c wmemset.c
MAN+=	nl_langinfo.3 setlocale.3

# citrus multibyte locale support
SRCS+=	___runetype_mb.c ___tolower_mb.c ___toupper_mb.c \
	multibyte.c rune.c runeglue.c runenone.c runetable.c setrunelocale.c
#SRCS+=	big5.c euc.c euctw.c iso2022.c mskanji.c utf2.c utf8.c

MLINKS+=setlocale.3 localeconv.3
