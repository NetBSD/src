#	from: @(#)Makefile.inc	5.1 (Berkeley) 2/18/91
#	$NetBSD: Makefile.inc,v 1.30 2000/12/25 23:30:58 itojun Exp $

# locale sources
.PATH: ${ARCHDIR}/locale ${.CURDIR}/locale

SRCS+=	_def_messages.c _def_monetary.c _def_numeric.c _def_time.c \
	ctypeio.c localeconv.c nl_langinfo.c \
	setlocale.c setlocale_sb.c
MAN+=	nl_langinfo.3 setlocale.3

MLINKS+=setlocale.3 localeconv.3

RUNE?=	no

.if ${RUNE} == "no"
# singlebyte locale - dummy
SRCS+=	multibyte_sb.c iswctype_sb.c
CPPFLAGS+=	-UWITH_RUNE
.else
# citrus multibyte locale support
SRCS+=	___runetype_mb.c ___tolower_mb.c ___toupper_mb.c iswctype.c \
	multibyte.c rune.c runeglue.c runenone.c runetable.c setrunelocale.c
CPPFLAGS+=	-DWITH_RUNE
#MODULE_SHLIB_MAJOR!=	. ${.CURDIR}/../localemod/shlib_version; echo $$major
#CPPFLAGS+=	-DDLRUNE -DRUNEMOD_MAJOR=${MODULE_SHLIB_MAJOR} \
#		-D_PATH_LOCALEMODULE=\"/usr/lib/runemodule\"
# to be dynamically loaded
#SRCS+=	big5.c euc.c euctw.c iso2022.c mskanji.c utf2.c utf8.c
.endif
MAN+=	iswalnum.3 towlower.3

MLINKS+=iswalnum.3 iswalpha.3 iswalnum.3 iswblank.3 \
	iswalnum.3 iswcntrl.3 iswalnum.3 iswdigit.3 \
	iswalnum.3 iswgraph.3 iswalnum.3 iswlower.3 \
	iswalnum.3 iswprint.3 iswalnum.3 iswpunct.3 \
	iswalnum.3 iswspace.3 iswalnum.3 iswupper.3 \
	iswalnum.3 iswxdigit.3
MLINKS+=towlower.3 towupper.3
