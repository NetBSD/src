#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.28 2000/12/22 05:21:41 itojun Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	ctypeio.c iswctype.c localeconv.c nl_langinfo.c \
	setlocale.c setlocale_sb.c \
	wcscat.c wcschr.c wcscmp.c wcscpy.c wcscspn.c wcslcat.c wcslcpy.c \
	wcslen.c wcsncat.c \
	wcsncmp.c wcsncpy.c wcspbrk.c wcsrchr.c wcsspn.c wcsstr.c wcswidth.c \
	wmemchr.c wmemcmp.c wmemcpy.c wmemmove.c wmemset.c
MAN+=	nl_langinfo.3 setlocale.3

MLINKS+=setlocale.3 localeconv.3

# citrus multibyte locale support
SRCS+=	___runetype_mb.c ___tolower_mb.c ___toupper_mb.c \
	multibyte.c rune.c runeglue.c runenone.c runetable.c setrunelocale.c
#MODULE_SHLIB_MAJOR!=	. ${.CURDIR}/../localemod/shlib_version; echo $$major
#CPPFLAGS+=	-DDLRUNE -DRUNEMOD_MAJOR=${MODULE_SHLIB_MAJOR} \
#		-D_PATH_LOCALEMODULE=\"/usr/lib/runemodule\"
# to be dynamically loaded
#SRCS+=	big5.c euc.c euctw.c iso2022.c mskanji.c utf2.c utf8.c
MAN+=	iswalnum.3 towlower.3 wmemchr.3

MLINKS+=iswalnum.3 iswalpha.3 iswalnum.3 iswblank.3 \
	iswalnum.3 iswcntrl.3 iswalnum.3 iswdigit.3 \
	iswalnum.3 iswgraph.3 iswalnum.3 iswlower.3 \
	iswalnum.3 iswprint.3 iswalnum.3 iswpunct.3 \
	iswalnum.3 iswspace.3 iswalnum.3 iswupper.3 \
	iswalnum.3 iswxdigit.3
MLINKS+=towlower.3 towupper.3
MLINKS+=wmemchr.3 wmemcmp.3 wmemchr.3 wmemcpy.3 \
	wmemchr.3 wmemmove.3 wmemchr.3 wmemset.3 \
	wmemchr.3 wcscat.3 wmemchr.3 wcschr.3 \
	wmemchr.3 wcscmp.3 wmemchr.3 wcscpy.3 \
	wmemchr.3 wcscspn.3 wmemchr.3 wcslcat.3 \
	wmemchr.3 wcslcpy.3 wmemchr.3 wcslen.3 \
	wmemchr.3 wcsncat.3 wmemchr.3 wcsncmp.3 \
	wmemchr.3 wcsncpy.3 wmemchr.3 wcspbrk.3 \
	wmemchr.3 wcsrchr.3 wmemchr.3 wcsspn.3 \
	wmemchr.3 wcsstr.3
