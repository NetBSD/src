#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.16.8.1 2000/05/28 22:41:03 minoura Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

.if !defined(WITH_RUNE)
WITH_RUNE= no
.endif
.if !defined(DLRUNE)
DLRUNE= no
.endif

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	localeconv.c nl_langinfo.c setlocale.c setlocale_sb.c \
	collate.c collcmp.c

.if ${WITH_RUNE} != "yes"
SRCS+=	ctypeio.c wcsmbs_sb.c
.else
# WITH_RUNE == "yes"
SRCS+=	multibyte.c iswctype.c tolowermb.c touppermb.c \
	runetable.c lconv.c localeconv.c runenone.c rune.c \
	setrunelocale.c runeglue.c runetypemb.c timelocal.c \
	wcscat.c wcschr.c wcscmp.c wcscpy.c wcscspn.c wcslen.c wcsncat.c \
	wcsncmp.c wcsncpy.c wcspbrk.c wcsrchr.c wcsspn.c wcsstr.c wmemchr.c \
	wmemcmp.c wmemcpy.c wmemmove.c wmemset.c wcswidth.c
CPPFLAGS+= -DWITH_RUNE
.if ${DLRUNE} == "yes"
CPPFLAGS+= -DDLRUNE -DRUNEMOD_MAJOR=1
.if !defined(OBJECT_FMT) || ${OBJECT_FMT} == "aout"
CPPFLAGS+= -DDLRUNE_AOUT
.endif # a.out
.else
# DLRUNE != "yes"
SRCS+= big5.c euc.c euctw.c iso2022.c mskanji.c utf2.c
CPPFLAGS+= -DXPG4
.endif # DLRUNE != "yes"
.endif # WITH_RUNE == "yes"

MAN+=	nl_langinfo.3 setlocale.3

MLINKS+=setlocale.3 localeconv.3

