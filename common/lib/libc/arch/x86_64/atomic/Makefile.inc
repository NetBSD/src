#	Makefile.inc,v 1.2.4.2 2008/01/09 01:21:14 matt Exp

.if defined(LIB) && (${LIB} == "kern" || ${LIB} == "c")

SRCS+=	atomic.S

.endif

.if defined(LIB) && (${LIB} == "c")

SRCS+=	atomic_init_cas.c

.endif
