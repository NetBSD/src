# $NetBSD: opt-jobs.mk,v 1.3 2023/09/10 11:00:55 rillig Exp $
#
# Tests for the -j command line option, which creates the targets in parallel.


# The option '-j <integer>' specifies the number of targets that can be made
# in parallel.
ARGS=		0 1 2 8 08 0x10 -5 1000
EXPECT.0=	illegal argument to -j -- must be positive integer! exit 2
EXPECT.1=	1
EXPECT.2=	2
EXPECT.8=	8
EXPECT.08=	illegal argument to -j -- must be positive integer! exit 2
EXPECT.0x10=	16
EXPECT.-5=	illegal argument to -j -- must be positive integer! exit 2
EXPECT.1000=	1000

.for arg in ${ARGS}
OUTPUT!=	${MAKE} -r -f /dev/null -j ${arg} -v .MAKE.JOBS 2>&1 || echo exit $$?
.  if ${OUTPUT:[2..-1]} != ${EXPECT.${arg}}
.      warning ${arg}:${.newline}    have: ${OUTPUT:[2..-1]}${.newline}    want: ${EXPECT.${arg}}
.  endif
.endfor


# The options '-j <float>' and '-j <integer>C' multiply the given number with
# the number of available CPUs.
ARGS=		0.0 .5 .5C 1C 1CPUs 1.2 .5e1C 07.5C 08.5C
EXPECT.0.0=	illegal argument to -j -- must be positive integer! exit 2
EXPECT..5=	<integer>		# rounded up to 1 if necessary
EXPECT..5C=	<integer>		# rounded up to 1 if necessary
EXPECT.1C=	<integer>
EXPECT.1CPUs=	<integer>
EXPECT.1.2=	<integer>
EXPECT..5e1C=	<integer>		# unlikely to occur in practice
EXPECT.07.5C=	<integer>
EXPECT.08.5C=	illegal argument to -j -- must be positive integer! exit 2

.if ${.MAKE.JOBS.C}
.  for arg in ${ARGS}
OUTPUT!=	${MAKE} -r -f /dev/null -j ${arg} -v .MAKE.JOBS 2>&1 || echo exit $$?
.    if ${OUTPUT:C,^[0-9]+$,numeric,W} == numeric
OUTPUT=		<integer>
.    endif
.    if ${OUTPUT:[2..-1]} != ${EXPECT.${arg}}
.      warning ${arg}:${.newline}    have: ${OUTPUT:[2..-1]}${.newline}    want: ${EXPECT.${arg}}
.    endif
.  endfor
.endif

all: .PHONY
