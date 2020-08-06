#	$NetBSD: Makefile,v 1.81 2020/08/06 05:32:26 rillig Exp $
#	@(#)Makefile	5.2 (Berkeley) 12/28/90

PROG=	make
SRCS=	arch.c buf.c compat.c cond.c dir.c for.c hash.c job.c lst.c main.c
SRCS+=	make.c make_malloc.c metachar.c parse.c
SRCS+=	str.c strlist.c suff.c targ.c trace.c var.c util.c

# Whether to generate a coverage report after running the tests.
USE_COVERAGE?=	no		# works only with gcc; clang9 fails to link
.if ${USE_COVERAGE} == "yes"
COPTS+=		--coverage -O0 -ggdb
LDADD+=		--coverage
CLEANFILES+=	${SRCS:.c=.gcda} ${SRCS:.c=.gcno} ${SRCS:=.gcov}
.endif

# Whether to compile using the Undefined Behavior Sanitizer (GCC, Clang).
USE_UBSAN?=	no
.if ${USE_UBSAN} == "yes"
COPTS+=		-fsanitize=undefined
LDADD+=		-fsanitize=undefined
.endif

USE_META?=	yes
.if ${USE_META:tl} != "no"

SRCS+=		meta.c
CPPFLAGS+=	-DUSE_META

USE_FILEMON?=	ktrace
.  if ${USE_FILEMON:tl} != "no"

.PATH:	${.CURDIR}/filemon
SRCS+=		filemon_${USE_FILEMON}.c
CPPFLAGS+=	-DUSE_FILEMON
CPPFLAGS+=	-DUSE_FILEMON_${USE_FILEMON:tu}

.    if ${USE_FILEMON} == "dev"
FILEMON_H?=	/usr/include/dev/filemon/filemon.h
.      if exists(${FILEMON_H}) && ${FILEMON_H:T} == "filemon.h"
COPTS.filemon_dev.c+= \
		-DHAVE_FILEMON_H -I${FILEMON_H:H}
.      endif
.    endif
.  endif
.endif

SUBDIR.roff+=	PSD.doc
.if make(obj) || make(clean)
SUBDIR+=	unit-tests
.endif

${SRCS:M*.c:.c=.o}: buf.h config.h dir.h hash.h job.h lst.h make.h make_malloc.h
${SRCS:M*.c:.c=.o}: meta.h metachar.h nonints.h pathnames.h sprite.h strlist.h trace.h

.include <bsd.prog.mk>
.include <bsd.subdir.mk>

CPPFLAGS+=	-DMAKE_NATIVE
COPTS.var.c+=	-Wno-cast-qual
COPTS.job.c+=	-Wno-format-nonliteral
COPTS.parse.c+=	-Wno-format-nonliteral
COPTS.var.c+=	-Wno-format-nonliteral

.if defined(TOOLDIR)
# This is a native NetBSD build, use libutil rather than the local emalloc etc.
CPPFLAGS+=	-DUSE_EMALLOC
LDADD+=		-lutil
DPADD+=		${LIBUTIL}
.endif

COPTS.arch.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.dir.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.main.c+=	${GCC_NO_FORMAT_TRUNCATION} ${GCC_NO_STRINGOP_TRUNCATION}
COPTS.meta.c+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.parse.c+=	${GCC_NO_FORMAT_TRUNCATION}

COPTS+=		-Wdeclaration-after-statement

# A simple unit-test driver to help catch regressions
test: .MAKE
	cd ${.CURDIR}/unit-tests \
	&& MAKEFLAGS= ${.MAKE} -r -m / TEST_MAKE=${TEST_MAKE:U${.OBJDIR}/${PROG:T}} ${.TARGET}
.if ${USE_COVERAGE} == yes
	gcov ${SRCS}
	sed -i 's,^\([^:]*\): *[0-9]*:,\1: ,' *.gcov
.endif

accept: .MAKE
	cd ${.CURDIR}/unit-tests && ${.MAKE} ${.TARGET}

retest:
	${.MAKE} -C ${.CURDIR}/unit-tests cleandir
.if ${USE_COVERAGE} == yes
	rm -f *.gcov *.gcda
.endif
	${.MAKE} test
