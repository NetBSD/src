#	$NetBSD: Makefile.inc,v 1.100 2024/07/24 08:55:08 kre Exp $
#	from: @(#)Makefile.inc	8.3 (Berkeley) 2/4/95

# stdlib sources
.PATH: ${ARCHDIR}/stdlib ${.CURDIR}/stdlib

SRCS+=	_env.c _rand48.c \
	a64l.c abort.c atexit.c atof.c atoi.c atol.c atoll.c \
	bsearch.c cxa_thread_atexit.c drand48.c exit.c \
	getenv.c getopt.c getopt_long.c getsubopt.c \
	hcreate.c heapsort.c imaxdiv.c insque.c jrand48.c l64a.c lldiv.c \
	lcong48.c lrand48.c lsearch.c merge.c mi_vector_hash.c mrand48.c \
	nrand48.c putenv.c qabs.c qdiv.c qsort.c posix_openpt.c pty.c \
	quick_exit.c radixsort.c rand.c rand_r.c random.c remque.c \
	reallocarr.c reallocarray.c seed48.c setenv.c srand48.c strsuftoll.c \
	strtoi.c strtou.c strtonum.c \
	strtoimax.c strtol.c strtoll.c strtoq.c strtoul.c strtoull.c \
	strtoumax.c strtouq.c system.c tdelete.c tfind.c tsearch.c twalk.c \
	unsetenv.c strfmon.c

# These are often replaced by MD .S versions
SRCS+=	abs.c labs.c llabs.c imaxabs.c
SRCS+=	div.c ldiv.c imaxdiv.c

# This might be replaced by erand48.c
SRCS+= erand48_ieee754.c

.if ${RUMPRUN} != "yes"
.   if (${USE_JEMALLOC} != "no")
.      if ${HAVE_JEMALLOC} > 100
.         include "${NETBSDSRCDIR}/external/bsd/jemalloc/lib/Makefile.inc"
.      else
SRCS+=	jemalloc.c aligned_alloc.c
.      endif 
.   else
SRCS+=	malloc.c
.   endif
.endif

CPPFLAGS.strtol.c+=	-I${LIBCDIR}/../../common/lib/libc/stdlib
CPPFLAGS.strtoq.c+=	-I${LIBCDIR}/../../common/lib/libc/stdlib
CPPFLAGS.strtouq.c+=	-I${LIBCDIR}/../../common/lib/libc/stdlib

.include "${ARCHDIR}/stdlib/Makefile.inc"

MAN+=	a64l.3 abort.3 abs.3 alloca.3 atexit.3 atof.3 atoi.3 atol.3 atoll.3 \
	at_quick_exit.3 \
	bsearch.3 \
	div.3 \
	exit.3 \
	getenv.3 getopt.3 getopt_long.3 getsubopt.3 grantpt.3 \
	hcreate.3 \
	insque.3 \
	jemalloc.3 \
	lsearch.3 \
	malloc.3 memory.3 mi_vector_hash.3 \
	posix_memalign.3 posix_openpt.3 ptsname.3 \
	qabs.3 qdiv.3 quick_exit.3 qsort.3 \
	radixsort.3 rand48.3 rand.3 random.3 reallocarr.3 reallocarray.3 \
	strfmon.3 strsuftoll.3 strtod.3 strtoi.3 strtol.3 strtou.3 strtoul.3 \
	strtonum.3 system.3 \
	tsearch.3 \
	unlockpt.3

strtou.3: strtoi.3
	${_MKTARGET_CREATE}
	${TOOL_SED} -e s/strtoi/strtou/g -e s/STRTOI/STRTOU/g \
	    -e 's/an intmax_t/a unitmax_t/g' -e 's/strtou 3/strtoi 3/g' \
	    -e s/intmax_t/uintmax_t/g -e s/%jd/%ju/g ${.ALLSRC} > ${.TARGET}
CLEANFILES += strtou.3

MLINKS+=a64l.3 l64a.3
MLINKS+=a64l.3 l64a_r.3
MLINKS+=abs.3 labs.3 \
	abs.3 llabs.3 \
	abs.3 imaxabs.3
MLINKS+=div.3 ldiv.3 \
	div.3 lldiv.3 \
	div.3 imaxdiv.3
MLINKS+=getenv.3 setenv.3 getenv.3 unsetenv.3 getenv.3 putenv.3
MLINKS+=getenv.3 getenv_r.3
MLINKS+=hcreate.3 hdestroy.3 hcreate.3 hsearch.3
MLINKS+=hcreate.3 hcreate_r.3 hcreate.3 hdestroy_r.3 hcreate.3 hsearch_r.3
MLINKS+=hcreate.3 hdestroy1.3 hcreate.3 hdestroy1_r.3
MLINKS+=insque.3 remque.3
MLINKS+=jemalloc.3 malloc.conf.5
MLINKS+=lsearch.3 lfind.3
MLINKS+=malloc.3 calloc.3 malloc.3 realloc.3 malloc.3 free.3
MLINKS+=posix_memalign.3 aligned_alloc.3
MLINKS+=qsort.3 heapsort.3 qsort.3 mergesort.3
MLINKS+=ptsname.3 ptsname_r.3
MLINKS+=rand.3 rand_r.3
MLINKS+=rand.3 srand.3
MLINKS+=rand48.3 drand48.3 rand48.3 erand48.3 rand48.3 lrand48.3
MLINKS+=rand48.3 mrand48.3 rand48.3 nrand48.3 rand48.3 jrand48.3
MLINKS+=rand48.3 srand48.3 rand48.3 seed48.3 rand48.3 lcong48.3
MLINKS+=random.3 initstate.3 random.3 setstate.3 random.3 srandom.3
MLINKS+=radixsort.3 sradixsort.3
MLINKS+=strfmon.3 strfmon_l.3
MLINKS+=strsuftoll.3 strsuftollx.3
MLINKS+=strtod.3 strtof.3 strtod.3 strtold.3
MLINKS+=strtoi.3 strtoi_l.3
MLINKS+=strtol.3 strtoimax.3
MLINKS+=strtol.3 strtoll.3
MLINKS+=strtol.3 strtoq.3
MLINKS+=strtou.3 strtou_l.3
MLINKS+=strtoul.3 strtoull.3
MLINKS+=strtoul.3 strtoumax.3
MLINKS+=strtoul.3 strtouq.3
MLINKS+=tsearch.3 tfind.3 tsearch.3 twalk.3 tsearch.3 tdelete.3

# lint(1) spuriously complains about `*s == CHAR_MAX' even though *s
# has type char.
LINTFLAGS.strfmon.c += -X 230

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
