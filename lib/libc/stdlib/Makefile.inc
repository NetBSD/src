#	from: @(#)Makefile.inc	5.6 (Berkeley) 6/4/91
#	$Id: Makefile.inc,v 1.21 1994/06/16 05:27:17 mycroft Exp $

# stdlib sources
.PATH: ${.CURDIR}/arch/${MACHINE_ARCH}/stdlib ${.CURDIR}/stdlib

SRCS+=	a64l.c abort.c atexit.c atoi.c atof.c atol.c bsearch.c calloc.c \
	exit.c getenv.c getopt.c heapsort.c l64a.c malloc.c merge.c \
	multibyte.c putenv.c qsort.c radixsort.c rand.c random.c realpath.c \
	setenv.c strtod.c strtol.c strtoq.c strtoul.c strtouq.c system.c \
	_rand48.c drand48.c erand48.c jrand48.c lcong48.c lrand48.c \
	mrand48.c nrand48.c seed48.c srand48.c

.if   (${MACHINE_ARCH} == "m68k")
SRCS+=	abs.S div.c labs.c ldiv.c
.elif (${MACHINE_ARCH} == "i386")
SRCS+=	abs.S div.S labs.S ldiv.S
.elif (${MACHINE_ARCH} == "ns32k")
SRCS+=	abs.S div.c labs.c ldiv.c
.elif (${MACHINE_ARCH} == "tahoe")
SRCS+=	abs.S div.c labs.c ldiv.c
.elif (${MACHINE_ARCH} == "vax")
SRCS+=	abs.S div.c labs.c ldiv.c
.else
SRCS+=	abs.c div.c labs.c ldiv.c
.endif

MAN3+=	abort.0 abs.0 alloca.0 atexit.0 atof.0 atoi.0 atol.0 bsearch.0 \
	calloc.0 div.0 exit.0 free.0 getenv.0 getopt.0 labs.0 ldiv.0 \
	malloc.0 memory.0 qsort.0 radixsort.0 rand48.0 rand.0 random.0 \
	realloc.0 realpath.0 strtod.0 strtol.0 strtoul.0 system.0

MLINKS+=getenv.3 setenv.3 getenv.3 unsetenv.3 getenv.3 putenv.3
MLINKS+=qsort.3 heapsort.3 qsort.3 mergesort.3
MLINKS+=rand.3 srand.3
MLINKS+=strtol.3 strtoq.3
MLINKS+=strtoul.3 strtouq.3
MLINKS+=random.3 initstate.3 random.3 setstate.3 random.3 srandom.3
MLINKS+=rand48.3 drand48.3 rand48.3 erand48.3 rand48.3 lrand48.3
MLINKS+=rand48.3 mrand48.3 rand48.3 nrand48.3 rand48.3 jrand48.3
MLINKS+=rand48.3 srand48.3 rand48.3 seed48.3 rand48.3 lcong48.3
