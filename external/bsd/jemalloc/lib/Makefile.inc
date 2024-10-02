#	$NetBSD: Makefile.inc,v 1.22 2024/10/02 18:13:17 christos Exp $

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
fxp.c \
hash.c \
hpa.c \
hpa_hooks.c \
hpdata.c \
hook.c \
inspect.c \
large.c \
log.c \
malloc_io.c \
mutex.c \
nstime.c \
pa.c \
pa_extra.c \
pac.c \
pai.c \
pages.c \
peak_event.c \
prof.c \
prof_data.c \
prof_recent.c \
psset.c \
rtree.c \
safety_check.c \
san.c \
san_bump.c \
sc.c \
sec.c \
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
# we need gnu11 for clang because of structure redefinitions.
# this is a c11 feature, that is accepted by gcc and gnu99
CPPFLAGS.${i}+=-I${JEMALLOC}/include -std=gnu11
# helps in tracking bad malloc/pointer usage, but has a serious
# performance penalty:
#   CPPFLAGS.${i}+= -DDJEMALLOC_PROTECT_NOSTD -DJEMALLOC_DEBUG
CPPFLAGS.${i}+=-DJEMALLOC_PROTECT_NOSTD
COPTS.${i}+= -fvisibility=hidden -funroll-loops
COPTS.${i}+= ${${ACTIVE_CC} == "clang":? -Wno-atomic-alignment -Wno-error=missing-noreturn:}
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

# jemalloc
.for i in ${JEMALLOC_SRCS}
LINTFLAGS.${i} += -X 86,89,117,132,161,191,193,207,217,226,261
.endfor

LINTFLAGS.arena.c += -X 298
LINTFLAGS.ckh.c += -X 239
LINTFLAGS.ctl.c += -X 135,239,298
LINTFLAGS.hpa.c += -X 135	# i386: alignment pai_s[4] -> hpa_shard_s[8]
LINTFLAGS.extent.c += -X 239
LINTFLAGS.jemalloc.c += -X 236	# XXX: lint bug, it is a constructor
LINTFLAGS.jemalloc.c += -X 141	# i386: lint is right, overflow is on purpose.
LINTFLAGS.pac.c += -X 135	# i386: alignment pai_s[4] -> hpa_shard_s[8]
LINTFLAGS.pages.c += -X 298
LINTFLAGS.prof_data.c += -X 236
LINTFLAGS.prof_recent.c += -X 42
LINTFLAGS.rtree.c += -X 239
LINTFLAGS.sc.c += -X 119,239
LINTFLAGS.sec.c += -X 247
LINTFLAGS.stats.c += -X 42,135,239,247
LINTFLAGS.tcache.c += -X 239,309
LINTFLAGS.thread_event.c += -X 239
