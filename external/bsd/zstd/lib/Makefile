# $NetBSD: Makefile,v 1.1 2024/10/27 22:48:51 christos Exp $	

NOLINT=yes	# XXX
WARNS?=		4
.include <bsd.init.mk>
USE_FORT?= yes	

CPPFLAGS+= -I${.OBJDIR}
DIST=${.CURDIR}/../dist
SRCDIR=${DIST}/lib

.PATH:	${SRCDIR}/decompress ${SRCDIR}/dictBuilder ${SRCDIR}/common \
	${SRCDIR}/compress ${SRCDIR}

LIB=	zstd

SRCS= \
cover.c \
debug.c \
divsufsort.c \
entropy_common.c \
error_private.c \
fastcover.c \
fse_compress.c \
fse_decompress.c \
hist.c \
huf_compress.c \
huf_decompress.c \
pool.c \
threading.c \
xxhash.c \
zdict.c \
zstd_common.c \
zstd_compress.c \
zstd_compress_literals.c \
zstd_compress_sequences.c \
zstd_compress_superblock.c \
zstd_ddict.c \
zstd_decompress.c \
zstd_decompress_block.c \
zstd_double_fast.c \
zstd_fast.c \
zstd_lazy.c \
zstd_ldm.c \
zstd_opt.c \
zstdmt_compress.c

.if ${MACHINE} == "amd64"
SRCS+= huf_decompress_${MACHINE}.S
.else
SRCS+= huf_decompress.c
.endif

# XXX: emmintrin.h causes lint pain
LINTFLAGS += -X 96,117,129,161,174,214,226,231,298

CPPFLAGS+= -I${SRCDIR} -DZSTD_MULTITHREAD

INCS=	zdict.h zstd.h zstd_errors.h  
INCSDIR=	/usr/include

LIBDPLIBS += pthread ${NETBSDSRCDIR}/lib/libpthread

.include <bsd.lib.mk>
