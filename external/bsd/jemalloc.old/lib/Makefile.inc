#	$NetBSD: Makefile.inc,v 1.2 2024/09/23 15:48:36 christos Exp $

JEMALLOC:=${.PARSEDIR}/..

JEMALLOC_SRCS+= \
jemalloc.c \
arena.c \
background_thread.c \
base.c \
bin.c \
bitmap.c \
ckh.c \
ctl.c \
divide.c \
extent.c \
extent_dss.c \
extent_mmap.c \
hash.c \
hooks.c \
large.c \
log.c \
malloc_io.c \
mutex.c \
mutex_pool.c \
nstime.c \
pages.c \
prng.c \
prof.c \
rtree.c \
stats.c \
sz.c \
tcache.c \
ticker.c \
tsd.c \
witness.c

.PATH: ${JEMALLOC}/dist/src ${JEMALLOC}/lib
.SUFFIXES: .3
.PATH.3: ${JEMALLOC}/dist/doc
.for i in ${JEMALLOC_SRCS}
# helps in tracking bad malloc/pointer usage, but has a serious
# performance penalty:
#   CPPFLAGS.${i}+=-I${JEMALLOC}/include -DJEMALLOC_PROTECT_NOSTD -DJEMALLOC_DEBUG
CPPFLAGS.${i}+=-I${JEMALLOC}/include -DJEMALLOC_PROTECT_NOSTD
COPTS.${i}+= -fvisibility=hidden -funroll-loops
COPTS.${i}+= ${${ACTIVE_CC} == "clang":? -Wno-atomic-alignment :}
LINTFLAGS.${i}+=	-X 231	# argument unused
LINTFLAGS.${i}+=	-X 220	# fallthrough on case statement
.endfor

COPTS.background_thread.c+=-Wno-error=stack-protector
COPTS.ctl.c+=-Wno-error=stack-protector
COPTS.stats.c+=-Wno-error=stack-protector
COPTS.tcache.c+=-Wno-error=stack-protector

SRCS+=${JEMALLOC_SRCS}

jemalloc.d jemalloc.pico jemalloc.o jemalloc.ln jemalloc.po jemalloc.go: \
    ${JEMALLOC}/dist/src/jemalloc.c

# jemalloc
LINTFLAGS.arena.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.background_thread.c += -X 86,117,132,161,191,193,207,217,226
LINTFLAGS.base.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.bin.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.bitmap.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.chk.c += -X 191,193,217,226,239
LINTFLAGS.ckh.c += -X 86,117,161,191,193,207,217,226,239,284
LINTFLAGS.ctl.c += -X 86,117,135,161,193,191,207,217,226
LINTFLAGS.divide.c += -X 226
LINTFLAGS.extent.c += -X 86,161,117,191,193,207,217,226
LINTFLAGS.extent_dss.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.extent_mmap.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.jemalloc.c += -X 86,117,161,191,193,207,217,226,236,307
LINTFLAGS.jemalloc.c += -X 141 # i386
LINTFLAGS.hooks.c += -X 226
LINTFLAGS.large.c += -X 86,161,117,191,193,207,217,226
LINTFLAGS.log.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.malloc_io.c += -X 86,117,161,191,193,207,217,226,284
LINTFLAGS.malloc_io.c += -X 132 # i386
LINTFLAGS.mutex.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.mutex_pool.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.nstime.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.pages.c += -X 86,117,161,191,193,207,217,226,298
LINTFLAGS.prng.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.prof.c += -X 86,117,161,181,191,193,207,217,226,236
LINTFLAGS.rtree.c += -X 86,117,161,191,193,207,217,226,239
LINTFLAGS.sz.c += -X 86,226
LINTFLAGS.stats.c += -X 42,86,117,135,161,191,193,207,217,226,247,351
LINTFLAGS.tcache.c += -X 86,117,161,191,193,207,217,226,239,309
LINTFLAGS.ticker.c += -X 86,117,161,191,193,207,217,226
LINTFLAGS.tsd.c += -X 86,117,161,191,193,207,217,226,346,351
LINTFLAGS.witness.c += -X 86,117,161,191,193,207,217,226
