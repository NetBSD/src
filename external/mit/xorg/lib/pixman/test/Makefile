
#	$NetBSD: Makefile,v 1.2 2022/11/13 21:57:12 mrg Exp $

NOINSTALL=1
NOMAN=1

.include <bsd.own.mk>

PROGS=					\
	oob-test			\
	infinite-loop			\
	trap-crasher			\
	fence-image-self-test		\
	region-translate-test		\
	fetch-test			\
	a1-trap-test			\
	prng-test			\
	radial-invalid			\
	pdf-op-test			\
	region-test			\
	combiner-test			\
	scaling-crash-test		\
	alpha-loop			\
	scaling-helpers-test		\
	thread-test			\
	rotate-test			\
	alphamap			\
	gradient-crash-test		\
	pixel-test			\
	matrix-test			\
	filter-reduction-test		\
	composite-traps-test		\
	region-contains-test		\
	glyph-test			\
	solid-test			\
	stress-test			\
	cover-test			\
	blitters-test			\
	affine-test			\
	scaling-test			\
	composite			\
	tolerance-test

ADDSRCS+=	utils.c utils-prng.c

.for _p in ${PROGS}
SRCS.${_p}=	${_p}.c ${ADDSRCS}
.endfor

CPPFLAGS+=	-I${X11SRCDIR.pixman}/pixman

LDFLAGS+=	-lpthread
LDADD+=		-lpixman-1

.include "../Makefile.cflags"

.PATH:	${X11SRCDIR.pixman}/test

check:
.for test in ${PROGS}
	echo -n "Starting ${test}: "; date
	./${test} && echo Passed: ${test}
.endfor
	echo -n "Done: "; date

.include <bsd.x11.mk>
.include <bsd.prog.mk>
