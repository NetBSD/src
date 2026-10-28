#	$NetBSD: brotli.mk,v 1.2 2026/10/28 21:14:14 mrg Exp $

# makefile fragment to build brotlidec for freetype2

.include "bsd.own.mk"

BROTLI_SRCS=	bit_reader.c decode.c huffman.c state.c
BROTLI_SRCS+=	constants.c context.c dictionary.c platform.c transform.c
BROTLI_SRCS+=	prefix.c static_init.c shared_dictionary.c

SRCS+=		${BROTLI_SRCS}

.for _f in ${BROTLI_SRCS}
CFLAGS.${_f}+=	-fcommon
CPPFLAGS+${_f}=	-DBROTLIDEC_SHARED_COMPILATION \
		-DBROTLI_HAVE_LOG2=1 \
		-DBROTLI_SHARED_COMPILATION \
		-DNDEBUG \
		-I${X11SRCDIR.brotli}/c/include
.endfor

.include "brotli-rename.mk"

.PATH: ${X11SRCDIR.brotli}/c/dec
.PATH: ${X11SRCDIR.brotli}/c/common
