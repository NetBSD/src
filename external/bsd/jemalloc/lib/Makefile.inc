#	$NetBSD: Makefile.inc,v 1.8 2019/03/29 15:13:44 christos Exp $

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
CPPFLAGS.${i}+=-I${JEMALLOC}/include -DJEMALLOC_PROTECT_NOSTD -DJEMALLOC_DEBUG
COPTS.${i}+= -fvisibility=hidden -funroll-loops 
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
