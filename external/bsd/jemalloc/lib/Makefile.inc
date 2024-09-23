#	$NetBSD: Makefile.inc,v 1.17 2024/09/23 15:03:47 christos Exp $

JEMALLOC:=${.PARSEDIR}/..

JEMALLOC_SRCS+= \
jemalloc.c \
arena.c \
background_thread.c \
base.c \
bin.c \
bin_info.c \
bitmap.c \
buf_writer.c \
cache_bin.c \
ckh.c \
ctl.c \
counter.c \
decay.c \
divide.c \
ecache.c \
edata.c \
edata_cache.c \
ehooks.c \
emap.c \
eset.c \
exp_grow.c \
extent.c \
extent_dss.c \
extent_mmap.c \
hash.c \
hpa_hooks.c \
hook.c \
large.c \
log.c \
malloc_io.c \
mutex.c \
nstime.c \
pa.c \
pac.c \
pai.c \
pages.c \
peak_event.c \
prof.c \
rtree.c \
san.c \
sc.c \
stats.c \
sz.c \
test_hooks.c \
tcache.c \
thread_event.c \
ticker.c \
tsd.c \
witness.c

.PATH: ${JEMALLOC}/dist/src ${JEMALLOC}/lib
.SUFFIXES: .3
.PATH.3: ${JEMALLOC}/dist/doc
.for i in ${JEMALLOC_SRCS}
CPPFLAGS.${i}+=-I${JEMALLOC}/include
# helps in tracking bad malloc/pointer usage, but has a serious
# performance penalty:
#   CPPFLAGS.${i}+= -DDJEMALLOC_PROTECT_NOSTD -DJEMALLOC_DEBUG
CPPFLAGS.${i}+=-DJEMALLOC_PROTECT_NOSTD
COPTS.${i}+= -fvisibility=hidden -funroll-loops
COPTS.${i}+= ${${ACTIVE_CC} == "clang":? -Wno-atomic-alignment :}
LINTFLAGS.${i}+=	-X 231	# argument unused
LINTFLAGS.${i}+=	-X 220	# fallthrough on case statement
.endfor

COPTS.background_thread.c+=-Wno-error=stack-protector
COPTS.ctl.c+=-Wno-error=stack-protector
COPTS.stats.c+=-Wno-error=stack-protector
COPTS.tcache.c+=-Wno-error=stack-protector
COPTS.tsd.c+=-Wno-error=missing-braces -Wno-missing-braces

SRCS+=${JEMALLOC_SRCS}

jemalloc.d jemalloc.pico jemalloc.o jemalloc.ln jemalloc.po jemalloc.go: \
    ${JEMALLOC}/dist/src/jemalloc.c
