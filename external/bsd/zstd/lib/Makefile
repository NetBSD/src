# $NetBSD: Makefile,v 1.7 2025/01/14 19:58:10 riastradh Exp $	

LIBISPRIVATE=pic
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

LIBDPLIBS += pthread ${NETBSDSRCDIR}/lib/libpthread

.if ${LIBISPRIVATE:Uno} == "no"
INCS=	zdict.h zstd.h zstd_errors.h  
INCSDIR=	/usr/include


PC=libzstd.pc
FILES+=			${PC}
FILESOWN_${PC}=		${BINOWN}
FILESGRP_${PC}=		${BINGRP}
FILESMODE_${PC}=	${NONBINMODE}
FILESDIR_${PC}=		/usr/lib/pkgconfig

CLEANFILES+=		${PC}

all: ${PC}
dependall: all

${PC}: ${PC}.in
	@${_MKTARGET_CREATE}
	@(V=$$( (echo '#include <zstd.h>'; echo ZSTD_LIB_VERSION) | \
	    ${CC} -E - | tail -1 | tr -d ' ') && \
	    ${TOOL_SED} -e s,@VERSION@,$$V, \
		-e s,@PREFIX@,/usr, -e s,@EXEC_PREFIX@,/usr/bin, \
		-e s,@INCLUDEDIR@,/usr/include, -e s,@LIBDIR@,/usr/lib, \
		-e s,@LIBS_PRIVATE@,, \
	    < ${.ALLSRC} > ${.TARGET})
.endif

.include <bsd.lib.mk>
