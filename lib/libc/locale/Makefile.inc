#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.45 2003/03/11 09:21:23 tshiozak Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	ctypeio.c localeconv.c nl_langinfo.c setlocale.c setlocale1.c \
	setlocale32.c __mb_cur_max.c wcscoll.c wcstod.c \
	wcstol.c wcstoll.c wcstoimax.c wcstoul.c wcstoull.c wcstoumax.c \
	wcsxfrm.c aliasname.c
MAN+=	nl_langinfo.3 setlocale.3

MLINKS+=setlocale.3 localeconv.3

.if ${CITRUS} == "no"
RUNE=		no
.endif

RUNE?=		yes

.if ${RUNE} == "no"
# singlebyte locale - dummy
SRCS+=	multibyte_sb.c iswctype_sb.c
CPPFLAGS+=-UWITH_RUNE
.else
# citrus multibyte locale support
# we have quirk for libc.a - see the last part of lib/libc/Makefile
CPPFLAGS+=-DWITH_RUNE -I${.CURDIR}
SRCS+=	___runetype_mb.c _wctrans.c iswctype.c multibyte_c90.c \
	multibyte_amd1.c rune.c runeglue.c runetable.c setrunelocale.c
.endif
MAN+=	btowc.3 iswalnum.3 iswctype.3 towlower.3 towctrans.3 mblen.3 mbrlen.3 \
	mbrtowc.3 mbsinit.3 mbsrtowcs.3 mbstowcs.3 mbtowc.3 wcrtomb.3 \
	wcsrtombs.3 wcstombs.3 wctob.3 wctomb.3 wctrans.3 wctype.3


MLINKS+=iswalnum.3 iswalpha.3 iswalnum.3 iswblank.3 \
	iswalnum.3 iswcntrl.3 iswalnum.3 iswdigit.3 \
	iswalnum.3 iswgraph.3 iswalnum.3 iswlower.3 \
	iswalnum.3 iswprint.3 iswalnum.3 iswpunct.3 \
	iswalnum.3 iswspace.3 iswalnum.3 iswupper.3 \
	iswalnum.3 iswxdigit.3
MLINKS+=towlower.3 towupper.3
