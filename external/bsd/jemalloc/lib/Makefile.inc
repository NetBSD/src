#	$NetBSD: Makefile.inc,v 1.15 2021/08/15 12:41:40 rillig Exp $

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

.if ${MACHINE_ARCH} == "vax"
# in merge_overlapping_regs, at regrename.c
COPTS.arena.c+=-O0
COPTS.extent.c+=-O0
.endif

SRCS+=${JEMALLOC_SRCS}

jemalloc.d jemalloc.pico jemalloc.o jemalloc.ln jemalloc.po jemalloc.go: \
    ${JEMALLOC}/dist/src/jemalloc.c
