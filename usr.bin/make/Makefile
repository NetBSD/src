#	$NetBSD: Makefile,v 1.96 2020/08/28 20:57:54 rillig Exp $
#	@(#)Makefile	5.2 (Berkeley) 12/28/90

PROG=	make
SRCS=	arch.c
SRCS+=  buf.c
SRCS+=  compat.c
SRCS+=  cond.c
SRCS+=  dir.c
SRCS+=  enum.c
SRCS+=  for.c
SRCS+=  hash.c
SRCS+=  job.c
SRCS+=  lst.c
SRCS+=  main.c
SRCS+=	make.c
SRCS+=  make_malloc.c
SRCS+=  metachar.c
SRCS+=  parse.c
SRCS+=	str.c
SRCS+=  strlist.c
SRCS+=  suff.c
SRCS+=  targ.c
SRCS+=  trace.c
SRCS+=  var.c
SRCS+=  util.c
HDRS=   buf.h
HDRS+=  config.h
HDRS+=  dir.h
HDRS+=  enum.h
HDRS+=  hash.h
HDRS+=  job.h
HDRS+=  lst.h
HDRS+=  make.h
HDRS+=  make_malloc.h
HDRS+=  meta.h
HDRS+=  metachar.h
HDRS+=  nonints.h
HDRS+=  pathnames.h
HDRS+=  strlist.h
HDRS+=  trace.h

# Whether to generate a coverage report after running the tests.
USE_COVERAGE?=	no		# works only with gcc; clang9 fails to link
.if ${USE_COVERAGE} == "yes"
GCOV?=		gcov
COPTS+=		--coverage -O0 -ggdb
LDADD+=		--coverage
.endif
CLEANFILES+=	*.gcda *.gcno *.gcov

# Whether to compile using the Undefined Behavior Sanitizer (GCC, Clang).
USE_UBSAN?=	no
.if ${USE_UBSAN} == "yes"
COPTS+=		-fsanitize=undefined
LDADD+=		-fsanitize=undefined
.endif

# Whether to compile with GCC 10 from pkgsrc, during development.
USE_GCC10?=	no
.if ${USE_GCC10} == "yes"
# CC is set further down in this file
COPTS+=		-Wno-attributes	# for abs and labs
COPTS.arch.c+=	-Wno-error=format-truncation
COPTS.dir.c+=	-Wno-error=format-truncation
COPTS.main.c+=	-Wno-error=format-truncation
COPTS.meta.c+=	-Wno-error=format-truncation
COPTS.parse.c+=	-Wno-error=format-truncation
.endif

# Whether to compile with GCC 9 from pkgsrc, during development.
USE_GCC9?=	no
.if ${USE_GCC9} == "yes"
# CC is set further down in this file
COPTS+=		-Wno-attributes	# for abs and labs
COPTS.arch.c+=	-Wno-error=format-truncation
COPTS.dir.c+=	-Wno-error=format-truncation
COPTS.main.c+=	-Wno-error=format-truncation
COPTS.meta.c+=	-Wno-error=format-truncation
COPTS.parse.c+=	-Wno-error=format-truncation
.endif

# Whether to compile with GCC 8 from pkgsrc, during development.
USE_GCC8?=	no
.if ${USE_GCC8} == "yes"
# CC is set further down in this file
COPTS+=		-Wno-attributes	# for abs and labs
COPTS.arch.c+=	-Wno-error=format-truncation
COPTS.dir.c+=	-Wno-error=format-truncation
COPTS.main.c+=	-Wno-error=format-truncation
COPTS.meta.c+=	-Wno-error=format-truncation
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

${SRCS:M*.c:.c=.o}: ${HDRS}

.include <bsd.prog.mk>
.include <bsd.subdir.mk>

CPPFLAGS+=	-DMAKE_NATIVE
COPTS.job.c+=	-Wno-format-nonliteral
COPTS.parse.c+=	-Wno-format-nonliteral
COPTS.var.c+=	-Wno-format-nonliteral

.if ${USE_GCC10} == "yes"
GCC9BASE?=	/usr/pkg/gcc10
CC=		${GCC10BASE}/bin/gcc
GCOV=		${GCC10BASE}/bin/gcov
.endif

.if ${USE_GCC9} == "yes"
GCC9BASE?=	/usr/pkg/gcc9
CC=		${GCC9BASE}/bin/gcc
GCOV=		${GCC9BASE}/bin/gcov
.endif

.if ${USE_GCC8} == "yes"
GCC8BASE?=	/usr/pkg/gcc8
CC=		${GCC8BASE}/bin/gcc
GCOV=		${GCC8BASE}/bin/gcov
.endif

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

# For -DCLEANUP and similar feature toggles.
CPPFLAGS+=	${USER_CPPFLAGS}
# For overriding -std=gnu99 or similar options.
CFLAGS+=	${USER_CFLAGS}

# A simple unit-test driver to help catch regressions
TEST_MAKE ?= ${.OBJDIR}/${PROG:T}
test: .MAKE
	cd ${.CURDIR}/unit-tests \
	&& MAKEFLAGS= ${TEST_MAKE} -r -m / TEST_MAKE=${TEST_MAKE} ${TESTS:DTESTS=${TESTS:Q}} ${.TARGET}
.if ${USE_COVERAGE} == yes
	${GCOV} ${GCOV_OPTS} ${SRCS}
	sed -i 's,^\([^:]*\): *[0-9]*:,\1: ,' *.gcov
.endif

accept sync-mi: .MAKE
	cd ${.CURDIR}/unit-tests && ${.MAKE} ${.TARGET}

retest:
	${.MAKE} -C ${.CURDIR}/unit-tests cleandir
.if ${USE_COVERAGE} == yes
	rm -f *.gcov *.gcda
.endif
	${.MAKE} test
